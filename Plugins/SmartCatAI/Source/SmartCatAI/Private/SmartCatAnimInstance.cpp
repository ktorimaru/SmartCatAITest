// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAnimInstance.h"
#include "SmartCatAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	// IK target calculation will be implemented here
	// This is where foot placement traces and target solving will occur
}
