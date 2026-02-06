// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_CatWander.h"
#include "SmartCatAIController.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_CatWander::UBTTask_CatWander()
{
	NodeName = "Cat Wander";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_CatWander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	// Get navigation system
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	// Find a random point within the wander radius
	FVector Origin = Pawn->GetActorLocation();
	FNavLocation RandomLocation;

	// Try to find a valid navigable point
	float Radius = FMath::RandRange(MinWanderRadius, MaxWanderRadius);
	bool bFound = NavSys->GetRandomReachablePointInRadius(Origin, Radius, RandomLocation);

	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}

	TargetLocation = RandomLocation.Location;
	bHasValidTarget = true;

	// Start moving to the location
	AIController->MoveToLocation(TargetLocation, AcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UBTTask_CatWander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Check if we've reached the destination
	APawn* Pawn = AIController->GetPawn();
	if (Pawn && bHasValidTarget)
	{
		float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetLocation);
		if (Distance <= AcceptanceRadius)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// Check if movement is still in progress
	EPathFollowingStatus::Type Status = AIController->GetMoveStatus();
	if (Status == EPathFollowingStatus::Idle)
	{
		// Movement stopped for some reason
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_CatWander::GetStaticDescription() const
{
	return FString::Printf(TEXT("Wander: %.0f - %.0f units"), MinWanderRadius, MaxWanderRadius);
}
