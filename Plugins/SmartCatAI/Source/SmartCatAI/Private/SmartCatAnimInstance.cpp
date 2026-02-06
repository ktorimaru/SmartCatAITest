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
	, IKMode(ECatIKMode::SlopeAdaptation)
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
	, GroundZ_FL(0.0f)
	, GroundZ_FR(0.0f)
	, GroundZ_BL(0.0f)
	, GroundZ_BR(0.0f)
	, SlopePitch(0.0f)
	, SlopeRoll(0.0f)
	, SlopeRotation(FRotator::ZeroRotator)
	, AverageGroundZ(0.0f)
	, ResidualOffset_FL(0.0f)
	, ResidualOffset_FR(0.0f)
	, ResidualOffset_BL(0.0f)
	, ResidualOffset_BR(0.0f)
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
	case ECatIKMode::SlopeAdaptation:
		UpdateSlopeAdaptationIK(DeltaSeconds);
		// SlopeAdaptation primarily uses mesh rotation, minimal per-foot IK
		IKAlpha = FMath::FInterpTo(IKAlpha, TargetAlpha, DeltaSeconds, IKInterpSpeed);
		PelvisAlpha = IKAlpha;
		break;

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

void USmartCatAnimInstance::UpdateSlopeAdaptationIK(float DeltaSeconds)
{
	// Slope Adaptation Mode:
	// 1. Sample ground height at each paw location
	// 2. Calculate slope pitch (front/back difference) and roll (left/right difference)
	// 3. Output rotation for mesh/root bone to match terrain slope
	// 4. Calculate residual offsets for optional per-foot IK fine-tuning

	FVector HitLocation, HitNormal;

	// Get bone positions
	FVector BoneFL = CachedMesh->GetSocketLocation(BoneName_FrontLeft);
	FVector BoneFR = CachedMesh->GetSocketLocation(BoneName_FrontRight);
	FVector BoneBL = CachedMesh->GetSocketLocation(BoneName_BackLeft);
	FVector BoneBR = CachedMesh->GetSocketLocation(BoneName_BackRight);

	// Sample ground Z at each paw location
	float RawGroundZ_FL = 0.0f, RawGroundZ_FR = 0.0f, RawGroundZ_BL = 0.0f, RawGroundZ_BR = 0.0f;
	bool bValidFL = false, bValidFR = false, bValidBL = false, bValidBR = false;

	if (TraceFootToGround(BoneName_FrontLeft, HitLocation, HitNormal))
	{
		RawGroundZ_FL = HitLocation.Z;
		bValidFL = true;
		GroundNormal_FL = HitNormal;
	}

	if (TraceFootToGround(BoneName_FrontRight, HitLocation, HitNormal))
	{
		RawGroundZ_FR = HitLocation.Z;
		bValidFR = true;
		GroundNormal_FR = HitNormal;
	}

	if (TraceFootToGround(BoneName_BackLeft, HitLocation, HitNormal))
	{
		RawGroundZ_BL = HitLocation.Z;
		bValidBL = true;
		GroundNormal_BL = HitNormal;
	}

	if (TraceFootToGround(BoneName_BackRight, HitLocation, HitNormal))
	{
		RawGroundZ_BR = HitLocation.Z;
		bValidBR = true;
		GroundNormal_BR = HitNormal;
	}

	// Interpolate ground Z values for smooth transitions
	GroundZ_FL = FMath::FInterpTo(GroundZ_FL, RawGroundZ_FL, DeltaSeconds, SlopeInterpSpeed);
	GroundZ_FR = FMath::FInterpTo(GroundZ_FR, RawGroundZ_FR, DeltaSeconds, SlopeInterpSpeed);
	GroundZ_BL = FMath::FInterpTo(GroundZ_BL, RawGroundZ_BL, DeltaSeconds, SlopeInterpSpeed);
	GroundZ_BR = FMath::FInterpTo(GroundZ_BR, RawGroundZ_BR, DeltaSeconds, SlopeInterpSpeed);

	// Calculate average ground heights for slope calculation
	float FrontAvgGround = (GroundZ_FL + GroundZ_FR) * 0.5f;
	float BackAvgGround = (GroundZ_BL + GroundZ_BR) * 0.5f;
	float LeftAvgGround = (GroundZ_FL + GroundZ_BL) * 0.5f;
	float RightAvgGround = (GroundZ_FR + GroundZ_BR) * 0.5f;

	AverageGroundZ = (GroundZ_FL + GroundZ_FR + GroundZ_BL + GroundZ_BR) * 0.25f;

	// Calculate slope pitch: positive = climbing (nose up), negative = descending (nose down)
	float SlopeHeightDiff = FrontAvgGround - BackAvgGround;
	float RawSlopePitch = FMath::RadiansToDegrees(FMath::Atan2(SlopeHeightDiff, BodyLength));
	RawSlopePitch = FMath::Clamp(RawSlopePitch, -MaxSlopePitch, MaxSlopePitch);

	// Calculate slope roll: positive = left side higher, negative = right side higher
	float RollHeightDiff = LeftAvgGround - RightAvgGround;
	float RawSlopeRoll = FMath::RadiansToDegrees(FMath::Atan2(RollHeightDiff, BodyWidth));
	RawSlopeRoll = FMath::Clamp(RawSlopeRoll, -MaxSlopeRoll, MaxSlopeRoll);

	// Interpolate slope angles for smooth rotation
	SlopePitch = FMath::FInterpTo(SlopePitch, RawSlopePitch, DeltaSeconds, SlopeInterpSpeed);
	SlopeRoll = FMath::FInterpTo(SlopeRoll, RawSlopeRoll, DeltaSeconds, SlopeInterpSpeed);

	// Build slope rotation (pitch and roll only, no yaw)
	SlopeRotation = FRotator(SlopePitch, 0.0f, SlopeRoll);

	// Calculate expected paw heights after slope rotation is applied
	// This uses simple trigonometry to estimate where paws would be after mesh rotates
	float PitchRad = FMath::DegreesToRadians(SlopePitch);
	float RollRad = FMath::DegreesToRadians(SlopeRoll);

	// Approximate Z adjustment from rotation for each corner
	// Front paws move up/down based on pitch, left/right paws based on roll
	float HalfLength = BodyLength * 0.5f;
	float HalfWidth = BodyWidth * 0.5f;

	float RotationAdjust_FL = HalfLength * FMath::Sin(PitchRad) + HalfWidth * FMath::Sin(RollRad);
	float RotationAdjust_FR = HalfLength * FMath::Sin(PitchRad) - HalfWidth * FMath::Sin(RollRad);
	float RotationAdjust_BL = -HalfLength * FMath::Sin(PitchRad) + HalfWidth * FMath::Sin(RollRad);
	float RotationAdjust_BR = -HalfLength * FMath::Sin(PitchRad) - HalfWidth * FMath::Sin(RollRad);

	// Calculate residual offsets (difference between actual ground and expected position after rotation)
	// These are for optional per-foot IK fine-tuning on uneven terrain
	float ExpectedZ_FL = AverageGroundZ + RotationAdjust_FL + FootHeight;
	float ExpectedZ_FR = AverageGroundZ + RotationAdjust_FR + FootHeight;
	float ExpectedZ_BL = AverageGroundZ + RotationAdjust_BL + FootHeight;
	float ExpectedZ_BR = AverageGroundZ + RotationAdjust_BR + FootHeight;

	ResidualOffset_FL = (GroundZ_FL + FootHeight) - ExpectedZ_FL;
	ResidualOffset_FR = (GroundZ_FR + FootHeight) - ExpectedZ_FR;
	ResidualOffset_BL = (GroundZ_BL + FootHeight) - ExpectedZ_BL;
	ResidualOffset_BR = (GroundZ_BR + FootHeight) - ExpectedZ_BR;

	// Clamp residual offsets
	ResidualOffset_FL = FMath::Clamp(ResidualOffset_FL, -MaxIKOffset, MaxIKOffset);
	ResidualOffset_FR = FMath::Clamp(ResidualOffset_FR, -MaxIKOffset, MaxIKOffset);
	ResidualOffset_BL = FMath::Clamp(ResidualOffset_BL, -MaxIKOffset, MaxIKOffset);
	ResidualOffset_BR = FMath::Clamp(ResidualOffset_BR, -MaxIKOffset, MaxIKOffset);

	// Set per-foot IK alphas based on residual offset magnitude
	// Only apply foot IK if residual is significant (uneven terrain)
	IKAlpha_FrontLeft = (FMath::Abs(ResidualOffset_FL) > ResidualIKThreshold) ? 1.0f : 0.0f;
	IKAlpha_FrontRight = (FMath::Abs(ResidualOffset_FR) > ResidualIKThreshold) ? 1.0f : 0.0f;
	IKAlpha_BackLeft = (FMath::Abs(ResidualOffset_BL) > ResidualIKThreshold) ? 1.0f : 0.0f;
	IKAlpha_BackRight = (FMath::Abs(ResidualOffset_BR) > ResidualIKThreshold) ? 1.0f : 0.0f;

	// Also populate the standard foot offsets for compatibility
	FootOffset_FL = ResidualOffset_FL;
	FootOffset_FR = ResidualOffset_FR;
	FootOffset_BL = ResidualOffset_BL;
	FootOffset_BR = ResidualOffset_BR;

	// Update pelvis data (use slope rotation)
	PelvisPitch = SlopePitch;
	PelvisRoll = SlopeRoll;
	PelvisRotation = SlopeRotation;
	PelvisOffsetZ = 0.0f; // Height adjustment handled by rotation, not offset

	// Calculate foot rotations from ground normals
	FootRotation_FL = CalculateFootRotationFromNormal(GroundNormal_FL);
	FootRotation_FR = CalculateFootRotationFromNormal(GroundNormal_FR);
	FootRotation_BL = CalculateFootRotationFromNormal(GroundNormal_BL);
	FootRotation_BR = CalculateFootRotationFromNormal(GroundNormal_BR);

	// Populate world-space IK targets (for residual foot IK if needed)
	IKFootTarget_FrontLeft = BoneFL + FVector(0, 0, ResidualOffset_FL);
	IKFootTarget_FrontRight = BoneFR + FVector(0, 0, ResidualOffset_FR);
	IKFootTarget_BackLeft = BoneBL + FVector(0, 0, ResidualOffset_BL);
	IKFootTarget_BackRight = BoneBR + FVector(0, 0, ResidualOffset_BR);

	// Build transforms
	IKFootTransform_FrontLeft = FTransform(FootRotation_FL.Quaternion(), IKFootTarget_FrontLeft);
	IKFootTransform_FrontRight = FTransform(FootRotation_FR.Quaternion(), IKFootTarget_FrontRight);
	IKFootTransform_BackLeft = FTransform(FootRotation_BL.Quaternion(), IKFootTarget_BackLeft);
	IKFootTransform_BackRight = FTransform(FootRotation_BR.Quaternion(), IKFootTarget_BackRight);
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
		float ComputedBodyLength = FVector::Dist2D(FrontMid, BackMid);

		if (ComputedBodyLength > 1.0f)
		{
			float HeightDiff = FrontAvgZ - BackAvgZ;
			PelvisPitch = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, ComputedBodyLength));
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
		float ComputedBodyWidth = FVector::Dist2D(LeftMid, RightMid);

		if (ComputedBodyWidth > 1.0f)
		{
			float HeightDiff = LeftAvgZ - RightAvgZ;
			PelvisRoll = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, ComputedBodyWidth));
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
	FString Header = TEXT("Time,Speed,");
	Header += TEXT("FL_Z,FL_GroundZ,FL_Diff,");
	Header += TEXT("FR_Z,FR_GroundZ,FR_Diff,");
	Header += TEXT("BL_Z,BL_GroundZ,BL_Diff,");
	Header += TEXT("BR_Z,BR_GroundZ,BR_Diff,");
	Header += TEXT("Bell_Z,Bell_GroundZ,Bell_Diff,");
	Header += TEXT("Jaw_Z,Jaw_GroundZ,Jaw_Diff\n");

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
	if (!CachedMesh)
	{
		CachedMesh = GetSkelMeshComponent();
		if (!CachedMesh)
		{
			return;
		}
	}

	// Get bone world positions
	FVector BoneFL = CachedMesh->GetSocketLocation(BoneName_FrontLeft);
	FVector BoneFR = CachedMesh->GetSocketLocation(BoneName_FrontRight);
	FVector BoneBL = CachedMesh->GetSocketLocation(BoneName_BackLeft);
	FVector BoneBR = CachedMesh->GetSocketLocation(BoneName_BackRight);
	FVector BoneBell = CachedMesh->GetSocketLocation(BoneName_Bell);
	FVector BoneJaw = CachedMesh->GetSocketLocation(BoneName_Jaw);

	// Trace to find ground at each location
	FVector HitLocation, HitNormal;
	float LocalGroundZ_FL = 0.0f, LocalGroundZ_FR = 0.0f, LocalGroundZ_BL = 0.0f, LocalGroundZ_BR = 0.0f;
	float LocalGroundZ_Bell = 0.0f, LocalGroundZ_Jaw = 0.0f;

	if (TraceFootToGround(BoneName_FrontLeft, HitLocation, HitNormal))
		LocalGroundZ_FL = HitLocation.Z;
	if (TraceFootToGround(BoneName_FrontRight, HitLocation, HitNormal))
		LocalGroundZ_FR = HitLocation.Z;
	if (TraceFootToGround(BoneName_BackLeft, HitLocation, HitNormal))
		LocalGroundZ_BL = HitLocation.Z;
	if (TraceFootToGround(BoneName_BackRight, HitLocation, HitNormal))
		LocalGroundZ_BR = HitLocation.Z;

	// Trace for Bell and Jaw (use same trace method but from those bone positions)
	if (CatCharacter)
	{
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(CatCharacter);
		FHitResult HitResult;

		// Bell trace
		FVector BellStart = BoneBell + FVector(0, 0, 50.0f);
		FVector BellEnd = BoneBell - FVector(0, 0, 200.0f);
		if (CatCharacter->GetWorld()->LineTraceSingleByChannel(HitResult, BellStart, BellEnd, TraceChannel, QueryParams))
			LocalGroundZ_Bell = HitResult.ImpactPoint.Z;

		// Jaw trace
		FVector JawStart = BoneJaw + FVector(0, 0, 50.0f);
		FVector JawEnd = BoneJaw - FVector(0, 0, 200.0f);
		if (CatCharacter->GetWorld()->LineTraceSingleByChannel(HitResult, JawStart, JawEnd, TraceChannel, QueryParams))
			LocalGroundZ_Jaw = HitResult.ImpactPoint.Z;
	}

	// Calculate differences (bone Z - ground Z)
	float Diff_FL = BoneFL.Z - LocalGroundZ_FL;
	float Diff_FR = BoneFR.Z - LocalGroundZ_FR;
	float Diff_BL = BoneBL.Z - LocalGroundZ_BL;
	float Diff_BR = BoneBR.Z - LocalGroundZ_BR;
	float Diff_Bell = BoneBell.Z - LocalGroundZ_Bell;
	float Diff_Jaw = BoneJaw.Z - LocalGroundZ_Jaw;

	// Get IK mode name
	FString IKModeName;
	switch (IKMode)
	{
	case ECatIKMode::Disabled: IKModeName = TEXT("OFF"); break;
	case ECatIKMode::SlopeAdaptation: IKModeName = TEXT("SLOPE"); break;
	case ECatIKMode::TerrainAdaptation: IKModeName = TEXT("TERRAIN"); break;
	case ECatIKMode::FullProcedural: IKModeName = TEXT("PROCEDURAL"); break;
	default: IKModeName = TEXT("UNKNOWN"); break;
	}

	// Print to screen
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::White,
			FString::Printf(TEXT("Speed: %.1f | IK: %s"),
				GroundSpeed, *IKModeName));

		// Show slope data if in SlopeAdaptation mode
		if (IKMode == ECatIKMode::SlopeAdaptation)
		{
			GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Orange,
				FString::Printf(TEXT("SLOPE: Pitch=%.1f  Roll=%.1f  AvgGround=%.1f"),
					SlopePitch, SlopeRoll, AverageGroundZ));

			GEngine->AddOnScreenDebugMessage(102, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("FL: Ground=%.1f  Residual=%.1f  Alpha=%.1f"),
					this->GroundZ_FL, ResidualOffset_FL, IKAlpha_FrontLeft));

			GEngine->AddOnScreenDebugMessage(103, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("FR: Ground=%.1f  Residual=%.1f  Alpha=%.1f"),
					this->GroundZ_FR, ResidualOffset_FR, IKAlpha_FrontRight));

			GEngine->AddOnScreenDebugMessage(104, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("BL: Ground=%.1f  Residual=%.1f  Alpha=%.1f"),
					this->GroundZ_BL, ResidualOffset_BL, IKAlpha_BackLeft));

			GEngine->AddOnScreenDebugMessage(105, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("BR: Ground=%.1f  Residual=%.1f  Alpha=%.1f"),
					this->GroundZ_BR, ResidualOffset_BR, IKAlpha_BackRight));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("FL: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneFL.Z, LocalGroundZ_FL, Diff_FL));

			GEngine->AddOnScreenDebugMessage(102, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("FR: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneFR.Z, LocalGroundZ_FR, Diff_FR));

			GEngine->AddOnScreenDebugMessage(103, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("BL: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneBL.Z, LocalGroundZ_BL, Diff_BL));

			GEngine->AddOnScreenDebugMessage(104, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("BR: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneBR.Z, LocalGroundZ_BR, Diff_BR));

			GEngine->AddOnScreenDebugMessage(105, 0.0f, FColor::Green,
				FString::Printf(TEXT("Bell: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneBell.Z, LocalGroundZ_Bell, Diff_Bell));

			GEngine->AddOnScreenDebugMessage(106, 0.0f, FColor::Magenta,
				FString::Printf(TEXT("Jaw: Z=%.1f  Ground=%.1f  Diff=%.1f"),
					BoneJaw.Z, LocalGroundZ_Jaw, Diff_Jaw));
		}
	}

	// Always append to file if recording
	if (bIsRecordingDebug)
	{
		static float RecordingTime = 0.0f;
		RecordingTime += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

		FString Row = FString::Printf(TEXT("%.3f,%.1f,"),
			RecordingTime, GroundSpeed);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f,"),
			BoneFL.Z, LocalGroundZ_FL, Diff_FL);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f,"),
			BoneFR.Z, LocalGroundZ_FR, Diff_FR);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f,"),
			BoneBL.Z, LocalGroundZ_BL, Diff_BL);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f,"),
			BoneBR.Z, LocalGroundZ_BR, Diff_BR);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f,"),
			BoneBell.Z, LocalGroundZ_Bell, Diff_Bell);

		Row += FString::Printf(TEXT("%.2f,%.2f,%.2f\n"),
			BoneJaw.Z, LocalGroundZ_Jaw, Diff_Jaw);

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
