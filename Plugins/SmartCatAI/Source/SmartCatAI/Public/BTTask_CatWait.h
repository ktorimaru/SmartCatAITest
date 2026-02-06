// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SmartCatAnimInstance.h"
#include "BTTask_CatWait.generated.h"

/**
 * Behavior Tree Task: Make the cat wait/idle with optional random actions
 */
UCLASS()
class SMARTCATAI_API UBTTask_CatWait : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CatWait();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Minimum wait time */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0"))
	float MinWaitTime = 2.0f;

	/** Maximum wait time */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0"))
	float MaxWaitTime = 8.0f;

	/** Chance to play a random idle action (0-1) */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0", ClampMax = "1"))
	float IdleActionChance = 0.3f;

	/** Possible idle actions to randomly trigger */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI")
	TArray<ECatAnimationAction> PossibleIdleActions;

private:
	/** Remaining wait time */
	float RemainingTime = 0.0f;

	/** Time until next idle action chance */
	float NextActionCheckTime = 0.0f;
};
