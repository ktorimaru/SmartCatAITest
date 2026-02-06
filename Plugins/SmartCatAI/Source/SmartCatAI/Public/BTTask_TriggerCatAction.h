// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SmartCatAnimInstance.h"
#include "BTTask_TriggerCatAction.generated.h"

/**
 * Behavior Tree Task: Trigger a cat animation action
 * Plays the specified action animation and waits for it to complete
 */
UCLASS()
class SMARTCATAI_API UBTTask_TriggerCatAction : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TriggerCatAction();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** The animation action to trigger */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI")
	ECatAnimationAction ActionToTrigger = ECatAnimationAction::Meow;

	/** Wait for the action to complete before succeeding */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI")
	bool bWaitForCompletion = true;

	/** Maximum time to wait for action to complete (0 = no limit) */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (EditCondition = "bWaitForCompletion"))
	float MaxWaitTime = 5.0f;

private:
	/** Track how long we've been waiting */
	float WaitTime = 0.0f;
};
