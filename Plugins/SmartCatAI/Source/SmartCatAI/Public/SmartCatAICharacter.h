// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SmartCatAnimInstance.h"
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

	/** Speed Up Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input")
	TObjectPtr<UInputAction> SpeedUpAction;

	/** Speed Down Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input")
	TObjectPtr<UInputAction> SpeedDownAction;

	// ============================================
	// Animation Action Inputs
	// ============================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> FlipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> HearAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> FocusAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> LayDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> SitAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> SleepAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> LickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> MeowAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SmartCatAI|Input|Actions")
	TObjectPtr<UInputAction> StretchAction;

	// ============================================
	// Speed Control
	// ============================================

	/** Amount to change speed per key press */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Movement")
	float SpeedAdjustAmount = 50.0f;

	/** Minimum walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Movement")
	float MinWalkSpeed = 50.0f;

	/** Maximum walk speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Movement")
	float MaxWalkSpeed = 800.0f;

private:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for look input */
	void Look(const FInputActionValue& Value);

	/** Called for speed up input */
	void SpeedUp(const FInputActionValue& Value);

	/** Called for speed down input */
	void SpeedDown(const FInputActionValue& Value);

	// Animation action handlers
	void OnFlip(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnHear(const FInputActionValue& Value);
	void OnFocus(const FInputActionValue& Value);
	void OnLayDown(const FInputActionValue& Value);
	void OnSit(const FInputActionValue& Value);
	void OnSleep(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);
	void OnLick(const FInputActionValue& Value);
	void OnMeow(const FInputActionValue& Value);
	void OnStretch(const FInputActionValue& Value);

	/** Helper to trigger animation action on the anim instance */
	void TriggerAnimationAction(ECatAnimationAction Action);
};
