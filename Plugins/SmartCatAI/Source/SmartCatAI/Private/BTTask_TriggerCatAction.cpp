// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_TriggerCatAction.h"
#include "SmartCatAIController.h"
#include "SmartCatAICharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_TriggerCatAction::UBTTask_TriggerCatAction()
{
	NodeName = "Trigger Cat Action";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_TriggerCatAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASmartCatAIController* CatController = Cast<ASmartCatAIController>(OwnerComp.GetAIOwner());
	if (!CatController)
	{
		return EBTNodeResult::Failed;
	}

	// Trigger the action
	CatController->TriggerAction(ActionToTrigger);

	// Reset wait timer
	WaitTime = 0.0f;

	if (!bWaitForCompletion)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_TriggerCatAction::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!bWaitForCompletion)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	WaitTime += DeltaSeconds;

	// Check for timeout
	if (MaxWaitTime > 0.0f && WaitTime >= MaxWaitTime)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Check if action is still playing
	ASmartCatAIController* CatController = Cast<ASmartCatAIController>(OwnerComp.GetAIOwner());
	if (CatController && !CatController->IsPlayingAction())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_TriggerCatAction::GetStaticDescription() const
{
	return FString::Printf(TEXT("Trigger Action: %s%s"),
		*UEnum::GetDisplayValueAsText(ActionToTrigger).ToString(),
		bWaitForCompletion ? TEXT(" (Wait)") : TEXT(""));
}
