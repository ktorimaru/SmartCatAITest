// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ASmartCatAICharacter::ASmartCatAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure capsule size for a cat
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 22.0f);

	// Configure character movement for quadruped
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		Movement->MaxWalkSpeed = 300.0f;
		Movement->MaxAcceleration = 1500.0f;
	}

	// Don't rotate character to controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ASmartCatAICharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply skeletal mesh if set
	if (CatSkeletalMesh)
	{
		GetMesh()->SetSkeletalMesh(CatSkeletalMesh);
	}

	// Apply animation class if set
	if (CatAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(CatAnimClass);
	}
}

void ASmartCatAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
