// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SmartCatAICharacter.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class SMARTCATAI_API ASmartCatAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASmartCatAICharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "SmartCatAI")
	USkeletalMeshComponent* GetCatMesh() const { return GetMesh(); }

protected:
	// ============================================
	// Mesh & Animation
	// ============================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Mesh")
	TObjectPtr<USkeletalMesh> CatSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Animation")
	TSubclassOf<UAnimInstance> CatAnimClass;

	// ============================================
	// Enhanced Input
	// ============================================

	/** Input Mapping Context for cat controls */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input")
	TObjectPtr<UInputMappingContext> CatMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input")
	TObjectPtr<UInputAction> LookAction;

private:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for look input */
	void Look(const FInputActionValue& Value);
};
