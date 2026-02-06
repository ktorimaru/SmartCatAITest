// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CatWait.h"
#include "SmartCatAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_CatWait::UBTTask_CatWait()
{
	NodeName = "Cat Wait";
	bNotifyTick = true;

	// Default idle actions
	PossibleIdleActions.Add(ECatAnimationAction::Meow);
	PossibleIdleActions.Add(ECatAnimationAction::Lick);
	PossibleIdleActions.Add(ECatAnimationAction::Stretch);
	PossibleIdleActions.Add(ECatAnimationAction::Hear);
}

EBTNodeResult::Type UBTTask_CatWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Set random wait time
	RemainingTime = FMath::RandRange(MinWaitTime, MaxWaitTime);
	NextActionCheckTime = FMath::RandRange(1.0f, 3.0f);

	return EBTNodeResult::InProgress;
}

void UBTTask_CatWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	RemainingTime -= DeltaSeconds;
	NextActionCheckTime -= DeltaSeconds;

	// Check for random idle action
	if (NextActionCheckTime <= 0.0f && PossibleIdleActions.Num() > 0)
	{
		NextActionCheckTime = FMath::RandRange(2.0f, 5.0f);

		if (FMath::FRand() < IdleActionChance)
		{
			ASmartCatAIController* CatController = Cast<ASmartCatAIController>(OwnerComp.GetAIOwner());
			if (CatController && !CatController->IsPlayingAction())
			{
				// Pick a random action
				int32 Index = FMath::RandRange(0, PossibleIdleActions.Num() - 1);
				CatController->TriggerAction(PossibleIdleActions[Index]);
			}
		}
	}

	// Check if wait time is over
	if (RemainingTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_CatWait::GetStaticDescription() const
{
	return FString::Printf(TEXT("Wait: %.1f - %.1f sec (%.0f%% action chance)"),
		MinWaitTime, MaxWaitTime, IdleActionChance * 100.0f);
}
