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

protected:
	void UpdateMovementState(float DeltaSeconds);
	void UpdateIKTargets(float DeltaSeconds);
};
