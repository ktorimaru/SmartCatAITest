// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAnimInstance.h"
#include "SmartCatAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

USmartCatAnimInstance::USmartCatAnimInstance()
	: CatCharacter(nullptr)
	, GroundSpeed(0.0f)
	, Velocity(FVector::ZeroVector)
	, bIsMoving(false)
	, bIsFalling(false)
	, IKFootTarget_FrontLeft(FVector::ZeroVector)
	, IKFootTarget_FrontRight(FVector::ZeroVector)
	, IKFootTarget_BackLeft(FVector::ZeroVector)
	, IKFootTarget_BackRight(FVector::ZeroVector)
	, IKAlpha_FrontLeft(1.0f)
	, IKAlpha_FrontRight(1.0f)
	, IKAlpha_BackLeft(1.0f)
	, IKAlpha_BackRight(1.0f)
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
	UpdateIKTargets(DeltaSeconds);
}

void USmartCatAnimInstance::UpdateMovementState(float DeltaSeconds)
{
	Velocity = CatCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	bIsMoving = GroundSpeed > 3.0f;

	if (UCharacterMovementComponent* Movement = CatCharacter->GetCharacterMovement())
	{
		bIsFalling = Movement->IsFalling();
	}
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

	// Determine if IK should be active
	bIKEnabled = ShouldEnableIK();

	// Target alpha based on IK enable state
	const float TargetAlpha = bIKEnabled ? 1.0f : 0.0f;

	if (!bIKEnabled)
	{
		// Immediately disable IK
		IKAlpha_FrontLeft = 0.0f;
		IKAlpha_FrontRight = 0.0f;
		IKAlpha_BackLeft = 0.0f;
		IKAlpha_BackRight = 0.0f;
		PelvisAlpha = 0.0f;
		return;
	}

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

	// Set foot targets directly (no interpolation)
	IKFootTarget_FrontLeft = RawFootLocation_FrontLeft;
	IKFootTarget_FrontRight = RawFootLocation_FrontRight;
	IKFootTarget_BackLeft = RawFootLocation_BackLeft;
	IKFootTarget_BackRight = RawFootLocation_BackRight;

	// Set pelvis offset directly (no interpolation)
	PelvisOffset = CalculatePelvisOffset();

	// Set IK alphas directly (no interpolation)
	IKAlpha_FrontLeft = TargetAlpha;
	IKAlpha_FrontRight = TargetAlpha;
	IKAlpha_BackLeft = TargetAlpha;
	IKAlpha_BackRight = TargetAlpha;
	PelvisAlpha = TargetAlpha;
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

	return true;
}
