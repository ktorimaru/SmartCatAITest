// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAnimInstance.h"
#include "SmartCatAICharacter.h"
#include "QuadrupedGaitCalculator.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

USmartCatAnimInstance::USmartCatAnimInstance()
	: CatCharacter(nullptr)
	, GroundSpeed(0.0f)
	, Velocity(FVector::ZeroVector)
	, bIsMoving(false)
	, bIsFalling(false)
	, MoveDirection(FVector::ForwardVector)
	, IKMode(ECatIKMode::TerrainAdaptation)
	, FootOffset_FL(0.0f)
	, FootOffset_FR(0.0f)
	, FootOffset_BL(0.0f)
	, FootOffset_BR(0.0f)
	, GroundNormal_FL(FVector::UpVector)
	, GroundNormal_FR(FVector::UpVector)
	, GroundNormal_BL(FVector::UpVector)
	, GroundNormal_BR(FVector::UpVector)
	, FootRotation_FL(FRotator::ZeroRotator)
	, FootRotation_FR(FRotator::ZeroRotator)
	, FootRotation_BL(FRotator::ZeroRotator)
	, FootRotation_BR(FRotator::ZeroRotator)
	, PelvisOffsetZ(0.0f)
	, PelvisPitch(0.0f)
	, PelvisRoll(0.0f)
	, PelvisRotation(FRotator::ZeroRotator)
	, IKFootTarget_FrontLeft(FVector::ZeroVector)
	, IKFootTarget_FrontRight(FVector::ZeroVector)
	, IKFootTarget_BackLeft(FVector::ZeroVector)
	, IKFootTarget_BackRight(FVector::ZeroVector)
	, IKAlpha_FrontLeft(1.0f)
	, IKAlpha_FrontRight(1.0f)
	, IKAlpha_BackLeft(1.0f)
	, IKAlpha_BackRight(1.0f)
	, IKAlpha(1.0f)
	, PelvisOffset(FVector::ZeroVector)
	, PelvisAlpha(1.0f)
{
}

void USmartCatAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CatCharacter = Cast<ASmartCatAICharacter>(TryGetPawnOwner());
}

void USmartCatAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CatCharacter)
	{
		CatCharacter = Cast<ASmartCatAICharacter>(TryGetPawnOwner());
		if (!CatCharacter)
		{
			return;
		}
	}

	UpdateMovementState(DeltaSeconds);
	UpdateGait(DeltaSeconds);
	UpdateIKTargets(DeltaSeconds);
}

void USmartCatAnimInstance::UpdateMovementState(float DeltaSeconds)
{
	Velocity = CatCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bIsMoving = GroundSpeed > 3.0f;

	// Calculate normalized movement direction
	if (bIsMoving)
	{
		MoveDirection = Velocity.GetSafeNormal2D();
	}
	// Keep last direction when stopped

	if (UCharacterMovementComponent* Movement = CatCharacter->GetCharacterMovement())
	{
		bIsFalling = Movement->IsFalling();
	}
}

void USmartCatAnimInstance::UpdateGait(float DeltaSeconds)
{
	// Update gait state using the shared calculator
	UQuadrupedGaitCalculator::UpdateGaitState(GaitState, GaitConfig, Velocity, DeltaSeconds);
	CurrentGait = GaitState.DetectedGait;
}

void USmartCatAnimInstance::UpdateIKTargets(float DeltaSeconds)
{
	// Cache mesh reference
	if (!CachedMesh)
	{
		CachedMesh = GetSkelMeshComponent();
		if (!CachedMesh)
		{
			return;
		}
	}

	// Get effective IK mode (may be overridden by state)
	ECatIKMode EffectiveMode = GetEffectiveIKMode();

	// Determine if IK should be active
	bIKEnabled = ShouldEnableIK() && (EffectiveMode != ECatIKMode::Disabled);

	// Target alpha based on IK enable state
	const float TargetAlpha = bIKEnabled ? 1.0f : 0.0f;

	if (!bIKEnabled)
	{
		// Smoothly disable IK
		IKAlpha = FMath::FInterpTo(IKAlpha, 0.0f, DeltaSeconds, IKInterpSpeed);
		IKAlpha_FrontLeft = IKAlpha;
		IKAlpha_FrontRight = IKAlpha;
		IKAlpha_BackLeft = IKAlpha;
		IKAlpha_BackRight = IKAlpha;
		PelvisAlpha = IKAlpha;

		// Reset terrain adaptation data when IK is disabled
		if (IKAlpha < 0.01f)
		{
			FootOffset_FL = 0.0f;
			FootOffset_FR = 0.0f;
			FootOffset_BL = 0.0f;
			FootOffset_BR = 0.0f;
			PelvisOffsetZ = 0.0f;
			PelvisPitch = 0.0f;
			PelvisRoll = 0.0f;
			PelvisRotation = FRotator::ZeroRotator;
		}
		return;
	}

	// Update based on IK mode
	switch (EffectiveMode)
	{
	case ECatIKMode::TerrainAdaptation:
		UpdateTerrainAdaptationIK(DeltaSeconds);
		// TerrainAdaptation sets per-foot alphas based on swing detection
		// Just update the global IKAlpha for pelvis
		IKAlpha = FMath::FInterpTo(IKAlpha, TargetAlpha, DeltaSeconds, IKInterpSpeed);
		PelvisAlpha = IKAlpha;
		break;

	case ECatIKMode::FullProcedural:
		UpdateProceduralIK(DeltaSeconds);
		// Procedural mode uses global alpha for all
		IKAlpha = FMath::FInterpTo(IKAlpha, TargetAlpha, DeltaSeconds, IKInterpSpeed);
		IKAlpha_FrontLeft = IKAlpha;
		IKAlpha_FrontRight = IKAlpha;
		IKAlpha_BackLeft = IKAlpha;
		IKAlpha_BackRight = IKAlpha;
		PelvisAlpha = IKAlpha;
		break;

	default:
		break;
	}
}

void USmartCatAnimInstance::UpdateTerrainAdaptationIK(float DeltaSeconds)
{
	// Terrain Adaptation Mode (Height-Based):
	// - Detect swing/stance by measuring paw height above ground
	// - If paw is above ground threshold → swing phase → alpha = 0 (let animation show)
	// - If paw is at/near ground → stance phase → alpha = 1 (apply IK to plant)

	FVector HitLocation, HitNormal;

	// Get bone positions (previous frame's output, but with correct alpha this reflects animation)
	FVector BoneFL = CachedMesh->GetSocketLocation(BoneName_FrontLeft);
	FVector BoneFR = CachedMesh->GetSocketLocation(BoneName_FrontRight);
	FVector BoneBL = CachedMesh->GetSocketLocation(BoneName_BackLeft);
	FVector BoneBR = CachedMesh->GetSocketLocation(BoneName_BackRight);

	// Trace and calculate height above ground for each foot
	float HeightAboveGround_FL = 0.0f;
	float HeightAboveGround_FR = 0.0f;
	float HeightAboveGround_BL = 0.0f;
	float HeightAboveGround_BR = 0.0f;

	// Front Left
	if (TraceFootToGround(BoneName_FrontLeft, HitLocation, HitNormal))
	{
		float GroundZ = HitLocation.Z + FootHeight;
		HeightAboveGround_FL = BoneFL.Z - GroundZ;
		RawFootOffset_FL = GroundZ - BoneFL.Z;  // Offset to reach ground
		RawFootOffset_FL = FMath::Clamp(RawFootOffset_FL, -MaxIKOffset, MaxIKOffset);
		GroundNormal_FL = HitNormal;
	}
	else
	{
		RawFootOffset_FL = 0.0f;
		GroundNormal_FL = FVector::UpVector;
		HeightAboveGround_FL = 999.0f;  // No ground found, treat as airborne
	}

	// Front Right
	if (TraceFootToGround(BoneName_FrontRight, HitLocation, HitNormal))
	{
		float GroundZ = HitLocation.Z + FootHeight;
		HeightAboveGround_FR = BoneFR.Z - GroundZ;
		RawFootOffset_FR = GroundZ - BoneFR.Z;
		RawFootOffset_FR = FMath::Clamp(RawFootOffset_FR, -MaxIKOffset, MaxIKOffset);
		GroundNormal_FR = HitNormal;
	}
	else
	{
		RawFootOffset_FR = 0.0f;
		GroundNormal_FR = FVector::UpVector;
		HeightAboveGround_FR = 999.0f;
	}

	// Back Left
	if (TraceFootToGround(BoneName_BackLeft, HitLocation, HitNormal))
	{
		float GroundZ = HitLocation.Z + FootHeight;
		HeightAboveGround_BL = BoneBL.Z - GroundZ;
		RawFootOffset_BL = GroundZ - BoneBL.Z;
		RawFootOffset_BL = FMath::Clamp(RawFootOffset_BL, -MaxIKOffset, MaxIKOffset);
		GroundNormal_BL = HitNormal;
	}
	else
	{
		RawFootOffset_BL = 0.0f;
		GroundNormal_BL = FVector::UpVector;
		HeightAboveGround_BL = 999.0f;
	}

	// Back Right
	if (TraceFootToGround(BoneName_BackRight, HitLocation, HitNormal))
	{
		float GroundZ = HitLocation.Z + FootHeight;
		HeightAboveGround_BR = BoneBR.Z - GroundZ;
		RawFootOffset_BR = GroundZ - BoneBR.Z;
		RawFootOffset_BR = FMath::Clamp(RawFootOffset_BR, -MaxIKOffset, MaxIKOffset);
		GroundNormal_BR = HitNormal;
	}
	else
	{
		RawFootOffset_BR = 0.0f;
		GroundNormal_BR = FVector::UpVector;
		HeightAboveGround_BR = 999.0f;
	}

	// Determine alpha based on height above ground
	// Above threshold = swing (alpha 0), at/below threshold = stance (alpha 1)
	float TargetAlpha_FL = (HeightAboveGround_FL > SwingPhaseHeightThreshold) ? 0.0f : 1.0f;
	float TargetAlpha_FR = (HeightAboveGround_FR > SwingPhaseHeightThreshold) ? 0.0f : 1.0f;
	float TargetAlpha_BL = (HeightAboveGround_BL > SwingPhaseHeightThreshold) ? 0.0f : 1.0f;
	float TargetAlpha_BR = (HeightAboveGround_BR > SwingPhaseHeightThreshold) ? 0.0f : 1.0f;

	// Interpolate foot offsets for smooth IK
	FootOffset_FL = FMath::FInterpTo(FootOffset_FL, RawFootOffset_FL, DeltaSeconds, IKInterpSpeed);
	FootOffset_FR = FMath::FInterpTo(FootOffset_FR, RawFootOffset_FR, DeltaSeconds, IKInterpSpeed);
	FootOffset_BL = FMath::FInterpTo(FootOffset_BL, RawFootOffset_BL, DeltaSeconds, IKInterpSpeed);
	FootOffset_BR = FMath::FInterpTo(FootOffset_BR, RawFootOffset_BR, DeltaSeconds, IKInterpSpeed);

	// Set per-foot IK alphas
	// Instant off during swing (alpha = 0) so animation foot lift shows
	// Smooth blend on during stance to avoid popping
	if (TargetAlpha_FL < 0.5f)
		IKAlpha_FrontLeft = 0.0f;
	else
		IKAlpha_FrontLeft = FMath::FInterpTo(IKAlpha_FrontLeft, TargetAlpha_FL, DeltaSeconds, FootIKBlendSpeed);

	if (TargetAlpha_FR < 0.5f)
		IKAlpha_FrontRight = 0.0f;
	else
		IKAlpha_FrontRight = FMath::FInterpTo(IKAlpha_FrontRight, TargetAlpha_FR, DeltaSeconds, FootIKBlendSpeed);

	if (TargetAlpha_BL < 0.5f)
		IKAlpha_BackLeft = 0.0f;
	else
		IKAlpha_BackLeft = FMath::FInterpTo(IKAlpha_BackLeft, TargetAlpha_BL, DeltaSeconds, FootIKBlendSpeed);

	if (TargetAlpha_BR < 0.5f)
		IKAlpha_BackRight = 0.0f;
	else
		IKAlpha_BackRight = FMath::FInterpTo(IKAlpha_BackRight, TargetAlpha_BR, DeltaSeconds, FootIKBlendSpeed);

	// Calculate foot rotations from ground normals
	FootRotation_FL = CalculateFootRotationFromNormal(GroundNormal_FL);
	FootRotation_FR = CalculateFootRotationFromNormal(GroundNormal_FR);
	FootRotation_BL = CalculateFootRotationFromNormal(GroundNormal_BL);
	FootRotation_BR = CalculateFootRotationFromNormal(GroundNormal_BR);

	// Calculate pelvis adjustment
	CalculatePelvisRotation();

	// Also update legacy PelvisOffset for compatibility
	PelvisOffset = FVector(0.0f, 0.0f, PelvisOffsetZ);

	// Populate world-space IK targets for ABP consumption
	// Target = ground position (where foot should plant when in stance)
	IKFootTarget_FrontLeft = BoneFL + FVector(0, 0, FootOffset_FL);
	IKFootTarget_FrontRight = BoneFR + FVector(0, 0, FootOffset_FR);
	IKFootTarget_BackLeft = BoneBL + FVector(0, 0, FootOffset_BL);
	IKFootTarget_BackRight = BoneBR + FVector(0, 0, FootOffset_BR);

	// Build transforms with rotation
	IKFootTransform_FrontLeft = FTransform(FootRotation_FL.Quaternion(), IKFootTarget_FrontLeft);
	IKFootTransform_FrontRight = FTransform(FootRotation_FR.Quaternion(), IKFootTarget_FrontRight);
	IKFootTransform_BackLeft = FTransform(FootRotation_BL.Quaternion(), IKFootTarget_BackLeft);
	IKFootTransform_BackRight = FTransform(FootRotation_BR.Quaternion(), IKFootTarget_BackRight);
}

void USmartCatAnimInstance::UpdateProceduralIK(float DeltaSeconds)
{
	// Full Procedural Mode:
	// - Uses gait calculator for procedural foot movement
	// - Combines with terrain traces
	// - Original behavior preserved for testing/special cases

	// Calculate gait outputs for each leg
	FQuadrupedLegGaitOutput GaitFL = UQuadrupedGaitCalculator::CalculateFrontLeftLeg(GaitState, GaitConfig, MoveDirection);
	FQuadrupedLegGaitOutput GaitFR = UQuadrupedGaitCalculator::CalculateFrontRightLeg(GaitState, GaitConfig, MoveDirection);
	FQuadrupedLegGaitOutput GaitBL = UQuadrupedGaitCalculator::CalculateBackLeftLeg(GaitState, GaitConfig, MoveDirection);
	FQuadrupedLegGaitOutput GaitBR = UQuadrupedGaitCalculator::CalculateBackRightLeg(GaitState, GaitConfig, MoveDirection);

	// Perform traces for each foot
	FVector HitLocation, HitNormal;

	// Front Left
	if (TraceFootToGround(BoneName_FrontLeft, HitLocation, HitNormal))
	{
		RawFootLocation_FrontLeft = HitLocation + FVector(0, 0, FootHeight);
		FVector BoneLocation = CachedMesh->GetSocketLocation(BoneName_FrontLeft);
		FootOffset_FrontLeft = CalculateFootOffset(RawFootLocation_FrontLeft, BoneLocation);
	}

	// Front Right
	if (TraceFootToGround(BoneName_FrontRight, HitLocation, HitNormal))
	{
		RawFootLocation_FrontRight = HitLocation + FVector(0, 0, FootHeight);
		FVector BoneLocation = CachedMesh->GetSocketLocation(BoneName_FrontRight);
		FootOffset_FrontRight = CalculateFootOffset(RawFootLocation_FrontRight, BoneLocation);
	}

	// Back Left
	if (TraceFootToGround(BoneName_BackLeft, HitLocation, HitNormal))
	{
		RawFootLocation_BackLeft = HitLocation + FVector(0, 0, FootHeight);
		FVector BoneLocation = CachedMesh->GetSocketLocation(BoneName_BackLeft);
		FootOffset_BackLeft = CalculateFootOffset(RawFootLocation_BackLeft, BoneLocation);
	}

	// Back Right
	if (TraceFootToGround(BoneName_BackRight, HitLocation, HitNormal))
	{
		RawFootLocation_BackRight = HitLocation + FVector(0, 0, FootHeight);
		FVector BoneLocation = CachedMesh->GetSocketLocation(BoneName_BackRight);
		FootOffset_BackRight = CalculateFootOffset(RawFootLocation_BackRight, BoneLocation);
	}

	// Apply gait offsets to foot targets
	IKFootTarget_FrontLeft = RawFootLocation_FrontLeft + GaitFL.PositionOffset;
	IKFootTarget_FrontRight = RawFootLocation_FrontRight + GaitFR.PositionOffset;
	IKFootTarget_BackLeft = RawFootLocation_BackLeft + GaitBL.PositionOffset;
	IKFootTarget_BackRight = RawFootLocation_BackRight + GaitBR.PositionOffset;

	// Build full effector transforms (location + rotation) for FABRIK
	IKFootTransform_FrontLeft = FTransform(GaitFL.EffectorRotation.Quaternion(), IKFootTarget_FrontLeft);
	IKFootTransform_FrontRight = FTransform(GaitFR.EffectorRotation.Quaternion(), IKFootTarget_FrontRight);
	IKFootTransform_BackLeft = FTransform(GaitBL.EffectorRotation.Quaternion(), IKFootTarget_BackLeft);
	IKFootTransform_BackRight = FTransform(GaitBR.EffectorRotation.Quaternion(), IKFootTarget_BackRight);

	// Set pelvis offset directly
	PelvisOffset = CalculatePelvisOffset();
	PelvisOffsetZ = PelvisOffset.Z;
}

bool USmartCatAnimInstance::TraceFootToGround(const FName& BoneName, FVector& OutHitLocation, FVector& OutHitNormal)
{
	if (!CachedMesh || !CatCharacter)
	{
		return false;
	}

	// Get bone location in world space
	FVector BoneLocation = CachedMesh->GetSocketLocation(BoneName);

	// Calculate trace start and end
	FVector TraceStart = BoneLocation + FVector(0, 0, TraceStartOffset);
	FVector TraceEnd = BoneLocation - FVector(0, 0, TraceEndOffset);

	// Setup trace parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CatCharacter);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	FHitResult HitResult;
	bool bHit = CatCharacter->GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTraces)
	{
		DrawDebugLine(
			CatCharacter->GetWorld(),
			TraceStart,
			TraceEnd,
			bHit ? FColor::Green : FColor::Red,
			false,
			-1.0f,
			0,
			1.0f
		);

		if (bHit)
		{
			DrawDebugSphere(
				CatCharacter->GetWorld(),
				HitResult.ImpactPoint,
				3.0f,
				8,
				FColor::Yellow,
				false,
				-1.0f
			);
		}
	}
#endif

	if (bHit)
	{
		OutHitLocation = HitResult.ImpactPoint;
		OutHitNormal = HitResult.ImpactNormal;
		return true;
	}

	// No hit - foot is in the air
	OutHitLocation = BoneLocation;
	OutHitNormal = FVector::UpVector;
	return false;
}

float USmartCatAnimInstance::CalculateFootOffset(const FVector& TraceHitLocation, const FVector& BoneWorldLocation)
{
	// Calculate the Z difference between where the foot should be and where it is
	float Offset = TraceHitLocation.Z - BoneWorldLocation.Z;

	// Clamp to maximum offset to prevent extreme stretching
	return FMath::Clamp(Offset, -MaxIKOffset, MaxIKOffset);
}

FVector USmartCatAnimInstance::CalculatePelvisOffset()
{
	// Find the minimum (most negative) foot offset
	// The pelvis needs to lower to accommodate the foot that needs to reach lowest
	float MinOffset = FMath::Min(
		FMath::Min(FootOffset_FrontLeft, FootOffset_FrontRight),
		FMath::Min(FootOffset_BackLeft, FootOffset_BackRight)
	);

	// Only apply pelvis adjustment for negative offsets (lowering the body)
	// When a foot needs to reach down, lower the pelvis
	if (MinOffset < 0.0f)
	{
		return FVector(0.0f, 0.0f, MinOffset);
	}

	return FVector::ZeroVector;
}

FVector USmartCatAnimInstance::InterpFootTarget(const FVector& Current, const FVector& Target, float DeltaSeconds)
{
	return FMath::VInterpTo(Current, Target, DeltaSeconds, IKInterpSpeed);
}

bool USmartCatAnimInstance::ShouldEnableIK() const
{
	// Disable IK when falling/jumping
	if (bIsFalling)
	{
		return false;
	}

	// Disable IK when moving too fast (running)
	if (GroundSpeed > IKDisableSpeedThreshold)
	{
		return false;
	}

	// Disable IK during certain action animations
	if (bIsPlayingAction)
	{
		switch (CurrentAction)
		{
		case ECatAnimationAction::Jump:
		case ECatAnimationAction::Fall:
		case ECatAnimationAction::Flip:
		case ECatAnimationAction::Attack:
			return false;
		default:
			break;
		}
	}

	return true;
}

ECatIKMode USmartCatAnimInstance::GetEffectiveIKMode() const
{
	// Force disable IK during certain actions regardless of mode setting
	if (bIsPlayingAction)
	{
		switch (CurrentAction)
		{
		case ECatAnimationAction::Jump:
		case ECatAnimationAction::Fall:
		case ECatAnimationAction::Flip:
		case ECatAnimationAction::Attack:
		case ECatAnimationAction::Sit:
		case ECatAnimationAction::LayDown:
		case ECatAnimationAction::Sleep:
			return ECatIKMode::Disabled;
		default:
			break;
		}
	}

	return IKMode;
}

void USmartCatAnimInstance::CalculatePelvisRotation()
{
	// Calculate pelvis Z offset (lowest foot determines body height)
	float MinOffset = FMath::Min(
		FMath::Min(FootOffset_FL, FootOffset_FR),
		FMath::Min(FootOffset_BL, FootOffset_BR)
	);

	// Only lower the pelvis, never raise it above animation
	PelvisOffsetZ = FMath::Min(MinOffset, 0.0f);

	// Calculate pitch from front/back height difference
	// Positive pitch = nose up (when front feet are higher than back)
	float FrontAvgZ = (FootOffset_FL + FootOffset_FR) * 0.5f;
	float BackAvgZ = (FootOffset_BL + FootOffset_BR) * 0.5f;

	// Get approximate body length for angle calculation
	// Use bone positions to estimate
	if (CachedMesh)
	{
		FVector FrontMid = (CachedMesh->GetSocketLocation(BoneName_FrontLeft) + CachedMesh->GetSocketLocation(BoneName_FrontRight)) * 0.5f;
		FVector BackMid = (CachedMesh->GetSocketLocation(BoneName_BackLeft) + CachedMesh->GetSocketLocation(BoneName_BackRight)) * 0.5f;
		float BodyLength = FVector::Dist2D(FrontMid, BackMid);

		if (BodyLength > 1.0f)
		{
			float HeightDiff = FrontAvgZ - BackAvgZ;
			PelvisPitch = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, BodyLength));
			PelvisPitch = FMath::Clamp(PelvisPitch, -15.0f, 15.0f); // Limit rotation
		}
	}

	// Calculate roll from left/right height difference
	// Positive roll = right side up (when left feet are higher than right)
	float LeftAvgZ = (FootOffset_FL + FootOffset_BL) * 0.5f;
	float RightAvgZ = (FootOffset_FR + FootOffset_BR) * 0.5f;

	if (CachedMesh)
	{
		FVector LeftMid = (CachedMesh->GetSocketLocation(BoneName_FrontLeft) + CachedMesh->GetSocketLocation(BoneName_BackLeft)) * 0.5f;
		FVector RightMid = (CachedMesh->GetSocketLocation(BoneName_FrontRight) + CachedMesh->GetSocketLocation(BoneName_BackRight)) * 0.5f;
		float BodyWidth = FVector::Dist2D(LeftMid, RightMid);

		if (BodyWidth > 1.0f)
		{
			float HeightDiff = LeftAvgZ - RightAvgZ;
			PelvisRoll = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, BodyWidth));
			PelvisRoll = FMath::Clamp(PelvisRoll, -10.0f, 10.0f); // Limit rotation
		}
	}

	// Combine into pelvis rotation
	PelvisRotation = FRotator(PelvisPitch, 0.0f, PelvisRoll);
}

FRotator USmartCatAnimInstance::CalculateFootRotationFromNormal(const FVector& GroundNormal) const
{
	// Calculate rotation to align foot with ground normal
	// Up vector to ground normal rotation
	FVector UpVector = FVector::UpVector;
	FQuat AlignmentQuat = FQuat::FindBetweenNormals(UpVector, GroundNormal);
	return AlignmentQuat.Rotator();
}

void USmartCatAnimInstance::TriggerAction(ECatAnimationAction Action)
{
	if (Action != ECatAnimationAction::None)
	{
		CurrentAction = Action;
		bIsPlayingAction = true;
		UE_LOG(LogTemp, Log, TEXT("Cat Action Triggered: %d"), static_cast<int32>(Action));
	}
}

void USmartCatAnimInstance::ClearAction()
{
	CurrentAction = ECatAnimationAction::None;
	bIsPlayingAction = false;
}

void USmartCatAnimInstance::StartRuntimeDebugRecording()
{
	bIsRecordingDebug = true;

	// Write header immediately to file
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("RuntimeIKDebug.csv");
	FString Header = TEXT("Time,Speed,IKMode,IKEnabled,");
	Header += TEXT("FL_Height,FL_Alpha,FL_Offset,");
	Header += TEXT("FR_Height,FR_Alpha,FR_Offset,");
	Header += TEXT("BL_Height,BL_Alpha,BL_Offset,");
	Header += TEXT("BR_Height,BR_Alpha,BR_Offset,");
	Header += TEXT("PelvisZ,PelvisPitch,PelvisRoll\n");

	FFileHelper::SaveStringToFile(Header, *FilePath);
	UE_LOG(LogTemp, Warning, TEXT("SmartCatAI: Started runtime debug recording to %s"), *FilePath);
}

void USmartCatAnimInstance::StopRuntimeDebugRecording()
{
	bIsRecordingDebug = false;
	UE_LOG(LogTemp, Warning, TEXT("SmartCatAI: Stopped runtime debug recording"));
}

void USmartCatAnimInstance::PrintDebugState()
{
	FString IKModeName;
	switch (IKMode)
	{
	case ECatIKMode::Disabled: IKModeName = TEXT("Disabled"); break;
	case ECatIKMode::TerrainAdaptation: IKModeName = TEXT("TerrainAdapt"); break;
	case ECatIKMode::FullProcedural: IKModeName = TEXT("Procedural"); break;
	default: IKModeName = TEXT("Unknown"); break;
	}

	// Calculate heights for display (same calculation as in UpdateTerrainAdaptationIK)
	FVector HitLocation, HitNormal;
	float Height_FL = 0.0f, Height_FR = 0.0f, Height_BL = 0.0f, Height_BR = 0.0f;

	if (CachedMesh)
	{
		FVector BoneFL = CachedMesh->GetSocketLocation(BoneName_FrontLeft);
		FVector BoneFR = CachedMesh->GetSocketLocation(BoneName_FrontRight);
		FVector BoneBL = CachedMesh->GetSocketLocation(BoneName_BackLeft);
		FVector BoneBR = CachedMesh->GetSocketLocation(BoneName_BackRight);

		if (TraceFootToGround(BoneName_FrontLeft, HitLocation, HitNormal))
			Height_FL = BoneFL.Z - (HitLocation.Z + FootHeight);
		if (TraceFootToGround(BoneName_FrontRight, HitLocation, HitNormal))
			Height_FR = BoneFR.Z - (HitLocation.Z + FootHeight);
		if (TraceFootToGround(BoneName_BackLeft, HitLocation, HitNormal))
			Height_BL = BoneBL.Z - (HitLocation.Z + FootHeight);
		if (TraceFootToGround(BoneName_BackRight, HitLocation, HitNormal))
			Height_BR = BoneBR.Z - (HitLocation.Z + FootHeight);
	}

	// Print to screen
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::White,
			FString::Printf(TEXT("Speed: %.1f | Mode: %s | IKEnabled: %d | SwingThreshold: %.1f"),
				GroundSpeed, *IKModeName, bIKEnabled ? 1 : 0, SwingPhaseHeightThreshold));

		GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("FL: Height=%.1f Alpha=%.2f Offset=%.1f %s"),
				Height_FL, IKAlpha_FrontLeft, FootOffset_FL, Height_FL > SwingPhaseHeightThreshold ? TEXT("[SWING]") : TEXT("[STANCE]")));

		GEngine->AddOnScreenDebugMessage(102, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("FR: Height=%.1f Alpha=%.2f Offset=%.1f %s"),
				Height_FR, IKAlpha_FrontRight, FootOffset_FR, Height_FR > SwingPhaseHeightThreshold ? TEXT("[SWING]") : TEXT("[STANCE]")));

		GEngine->AddOnScreenDebugMessage(103, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("BL: Height=%.1f Alpha=%.2f Offset=%.1f %s"),
				Height_BL, IKAlpha_BackLeft, FootOffset_BL, Height_BL > SwingPhaseHeightThreshold ? TEXT("[SWING]") : TEXT("[STANCE]")));

		GEngine->AddOnScreenDebugMessage(104, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("BR: Height=%.1f Alpha=%.2f Offset=%.1f %s"),
				Height_BR, IKAlpha_BackRight, FootOffset_BR, Height_BR > SwingPhaseHeightThreshold ? TEXT("[SWING]") : TEXT("[STANCE]")));

		GEngine->AddOnScreenDebugMessage(105, 0.0f, FColor::Green,
			FString::Printf(TEXT("Pelvis: Z=%.1f Pitch=%.1f Roll=%.1f"),
				PelvisOffsetZ, PelvisPitch, PelvisRoll));
	}

	// Always append to file if recording (write immediately, don't buffer)
	if (bIsRecordingDebug)
	{
		static float RecordingTime = 0.0f;
		RecordingTime += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

		FString Row = FString::Printf(TEXT("%.3f,%.1f,%s,%d,"),
			RecordingTime, GroundSpeed, *IKModeName, bIKEnabled ? 1 : 0);

		Row += FString::Printf(TEXT("%.2f,%.3f,%.2f,"),
			Height_FL, IKAlpha_FrontLeft, FootOffset_FL);

		Row += FString::Printf(TEXT("%.2f,%.3f,%.2f,"),
			Height_FR, IKAlpha_FrontRight, FootOffset_FR);

		Row += FString::Printf(TEXT("%.2f,%.3f,%.2f,"),
			Height_BL, IKAlpha_BackLeft, FootOffset_BL);

		Row += FString::Printf(TEXT("%.2f,%.3f,%.2f,"),
			Height_BR, IKAlpha_BackRight, FootOffset_BR);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f\n"),
			PelvisOffsetZ, PelvisPitch, PelvisRoll);

		// Append directly to file
		FString FilePath = FPaths::ProjectSavedDir() / TEXT("RuntimeIKDebug.csv");
		FFileHelper::SaveStringToFile(Row, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
	}
}

void USmartCatAnimInstance::ExportGaitDataToCSV(float MinSpeed, float MaxSpeed, float SpeedStep, float TimeStep, float CycleDuration)
{
	// Build CSV content
	FString CSVContent;

	// Header
	CSVContent += TEXT("Speed,Time,Gait,Phase,");
	CSVContent += TEXT("FL_Phase,FL_Swinging,FL_SwingProgress,FL_LiftHeight,FL_StrideOffset,");
	CSVContent += TEXT("FR_Phase,FR_Swinging,FR_SwingProgress,FR_LiftHeight,FR_StrideOffset,");
	CSVContent += TEXT("BL_Phase,BL_Swinging,BL_SwingProgress,BL_LiftHeight,BL_StrideOffset,");
	CSVContent += TEXT("BR_Phase,BR_Swinging,BR_SwingProgress,BR_LiftHeight,BR_StrideOffset\n");

	// Iterate through speeds
	for (float Speed = MinSpeed; Speed <= MaxSpeed; Speed += SpeedStep)
	{
		// Create a fresh gait state for this speed
		FQuadrupedGaitState TestState;
		FVector TestVelocity = FVector(Speed, 0, 0); // Forward velocity
		FVector MoveDir = FVector(1, 0, 0);

		// Simulate for CycleDuration seconds
		for (float Time = 0.0f; Time < CycleDuration; Time += TimeStep)
		{
			// Update gait state
			UQuadrupedGaitCalculator::UpdateGaitState(TestState, GaitConfig, TestVelocity, TimeStep);

			// Calculate leg outputs
			FQuadrupedLegGaitOutput FL = UQuadrupedGaitCalculator::CalculateFrontLeftLeg(TestState, GaitConfig, MoveDir);
			FQuadrupedLegGaitOutput FR = UQuadrupedGaitCalculator::CalculateFrontRightLeg(TestState, GaitConfig, MoveDir);
			FQuadrupedLegGaitOutput BL = UQuadrupedGaitCalculator::CalculateBackLeftLeg(TestState, GaitConfig, MoveDir);
			FQuadrupedLegGaitOutput BR = UQuadrupedGaitCalculator::CalculateBackRightLeg(TestState, GaitConfig, MoveDir);

			// Get gait name
			FString GaitName;
			switch (TestState.DetectedGait)
			{
			case EQuadrupedGait::Stroll: GaitName = TEXT("Stroll"); break;
			case EQuadrupedGait::Walk: GaitName = TEXT("Walk"); break;
			case EQuadrupedGait::Trot: GaitName = TEXT("Trot"); break;
			case EQuadrupedGait::Gallop: GaitName = TEXT("Gallop"); break;
			default: GaitName = TEXT("Unknown"); break;
			}

			// Write row
			CSVContent += FString::Printf(TEXT("%.1f,%.3f,%s,%.3f,"),
				Speed, Time, *GaitName, TestState.GaitCyclePhase);

			// FL data
			CSVContent += FString::Printf(TEXT("%.3f,%d,%.3f,%.2f,%.2f,"),
				FL.StepPhase, FL.bIsSwinging ? 1 : 0, FL.SwingProgress, FL.LiftHeight, FL.StrideOffset);

			// FR data
			CSVContent += FString::Printf(TEXT("%.3f,%d,%.3f,%.2f,%.2f,"),
				FR.StepPhase, FR.bIsSwinging ? 1 : 0, FR.SwingProgress, FR.LiftHeight, FR.StrideOffset);

			// BL data
			CSVContent += FString::Printf(TEXT("%.3f,%d,%.3f,%.2f,%.2f,"),
				BL.StepPhase, BL.bIsSwinging ? 1 : 0, BL.SwingProgress, BL.LiftHeight, BL.StrideOffset);

			// BR data
			CSVContent += FString::Printf(TEXT("%.3f,%d,%.3f,%.2f,%.2f\n"),
				BR.StepPhase, BR.bIsSwinging ? 1 : 0, BR.SwingProgress, BR.LiftHeight, BR.StrideOffset);
		}
	}

	// Write to file
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("GaitData.csv");
	if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Gait data exported to: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to export gait data to: %s"), *FilePath);
	}
}
