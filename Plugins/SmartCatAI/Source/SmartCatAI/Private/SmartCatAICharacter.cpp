// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

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

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (CatMappingContext)
			{
				Subsystem->AddMappingContext(CatMappingContext, 0);
			}
		}
	}
}

void ASmartCatAICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASmartCatAICharacter::Move);
		}

		// Looking
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASmartCatAICharacter::Look);
		}

		// Speed Up
		if (SpeedUpAction)
		{
			EnhancedInputComponent->BindAction(SpeedUpAction, ETriggerEvent::Started, this, &ASmartCatAICharacter::SpeedUp);
		}

		// Speed Down
		if (SpeedDownAction)
		{
			EnhancedInputComponent->BindAction(SpeedDownAction, ETriggerEvent::Started, this, &ASmartCatAICharacter::SpeedDown);
		}
	}
}

void ASmartCatAICharacter::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Get forward and right vectors based on controller rotation
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASmartCatAICharacter::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASmartCatAICharacter::SpeedUp(const FInputActionValue& Value)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		float NewSpeed = FMath::Clamp(Movement->MaxWalkSpeed + SpeedAdjustAmount, MinWalkSpeed, MaxWalkSpeed);
		Movement->MaxWalkSpeed = NewSpeed;
		UE_LOG(LogTemp, Log, TEXT("Walk Speed: %.0f"), NewSpeed);
	}
}

void ASmartCatAICharacter::SpeedDown(const FInputActionValue& Value)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		float NewSpeed = FMath::Clamp(Movement->MaxWalkSpeed - SpeedAdjustAmount, MinWalkSpeed, MaxWalkSpeed);
		Movement->MaxWalkSpeed = NewSpeed;
		UE_LOG(LogTemp, Log, TEXT("Walk Speed: %.0f"), NewSpeed);
	}
}

void ASmartCatAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
