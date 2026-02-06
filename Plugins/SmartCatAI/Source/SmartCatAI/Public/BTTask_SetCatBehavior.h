// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SmartCatAIController.h"
#include "BTTask_SetCatBehavior.generated.h"

/**
 * Behavior Tree Task: Set the cat's current behavior state
 */
UCLASS()
class SMARTCATAI_API UBTTask_SetCatBehavior : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetCatBehavior();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** The behavior to set */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI")
	ECatBehavior BehaviorToSet = ECatBehavior::Idle;
};
