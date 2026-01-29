// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SmartCatAnimInstance.generated.h"

class ASmartCatAICharacter;

UCLASS()
class SMARTCATAI_API USmartCatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	USmartCatAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Animation")
	ASmartCatAICharacter* GetSmartCatCharacter() const { return CatCharacter; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Animation")
	TObjectPtr<ASmartCatAICharacter> CatCharacter;

	// Movement state for blend spaces
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	bool bIsMoving;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	bool bIsFalling;

	// IK foot effector targets (world space)
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector IKFootTarget_FrontLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector IKFootTarget_FrontRight;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector IKFootTarget_BackLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector IKFootTarget_BackRight;

	// IK blend weights (0 = animation, 1 = IK target)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_FrontLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_FrontRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_BackLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_BackRight;

	// Pelvis offset for body adjustment
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector PelvisOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PelvisAlpha;

	// ============================================
	// IK Configuration
	// ============================================

	/** Bone names for foot placement tracing */
	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_FrontLeft = TEXT("Cat-Shorthair-L-Hand");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_FrontRight = TEXT("Cat-Shorthair-R-Hand");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_BackLeft = TEXT("Cat-Shorthair-L-Toe0");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_BackRight = TEXT("Cat-Shorthair-R-Toe0");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_Pelvis = TEXT("Cat-Shorthair-Pelvis");

	/** How far above the foot bone to start the trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float TraceStartOffset = 50.0f;

	/** How far below the foot bone to end the trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float TraceEndOffset = 75.0f;

	/** Collision channel for foot traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Maximum IK adjustment distance (prevents extreme stretching) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float MaxIKOffset = 30.0f;

	/** Speed of IK target interpolation (higher = faster response) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float IKInterpSpeed = 15.0f;

	/** Speed of pelvis offset interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float PelvisInterpSpeed = 10.0f;

	/** Minimum ground speed to enable IK (disables during fast movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float IKDisableSpeedThreshold = 400.0f;

	/** Foot height offset from ground surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float FootHeight = 2.0f;

	// ============================================
	// IK Debug
	// ============================================

	/** Enable debug visualization of IK traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Debug")
	bool bDrawDebugTraces = false;

protected:
	void UpdateMovementState(float DeltaSeconds);
	void UpdateIKTargets(float DeltaSeconds);

private:
	/** Perform a single foot trace and return the hit location */
	bool TraceFootToGround(const FName& BoneName, FVector& OutHitLocation, FVector& OutHitNormal);

	/** Calculate the foot offset needed based on trace result */
	float CalculateFootOffset(const FVector& TraceHitLocation, const FVector& BoneWorldLocation);

	/** Calculate pelvis offset based on all foot offsets */
	FVector CalculatePelvisOffset();

	/** Smoothly interpolate a foot target */
	FVector InterpFootTarget(const FVector& Current, const FVector& Target, float DeltaSeconds);

	/** Check if IK should be active based on movement state */
	bool ShouldEnableIK() const;

	/** Raw trace hit locations before interpolation */
	FVector RawFootLocation_FrontLeft;
	FVector RawFootLocation_FrontRight;
	FVector RawFootLocation_BackLeft;
	FVector RawFootLocation_BackRight;

	/** Foot offsets from original position (Z only) */
	float FootOffset_FrontLeft = 0.0f;
	float FootOffset_FrontRight = 0.0f;
	float FootOffset_BackLeft = 0.0f;
	float FootOffset_BackRight = 0.0f;

	/** Cached reference to skeletal mesh component */
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	/** Whether IK is currently enabled */
	bool bIKEnabled = false;
};
