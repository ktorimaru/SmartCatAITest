// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_SetCatBehavior.h"
#include "SmartCatAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetCatBehavior::UBTTask_SetCatBehavior()
{
	NodeName = "Set Cat Behavior";
}

EBTNodeResult::Type UBTTask_SetCatBehavior::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASmartCatAIController* CatController = Cast<ASmartCatAIController>(OwnerComp.GetAIOwner());
	if (!CatController)
	{
		return EBTNodeResult::Failed;
	}

	CatController->TriggerBehavior(BehaviorToSet);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetCatBehavior::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set Behavior: %s"),
		*UEnum::GetDisplayValueAsText(BehaviorToSet).ToString());
}
