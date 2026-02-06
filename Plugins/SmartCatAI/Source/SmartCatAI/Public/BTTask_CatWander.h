// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CatWander.generated.h"

/**
 * Behavior Tree Task: Make the cat wander to a random nearby location
 */
UCLASS()
class SMARTCATAI_API UBTTask_CatWander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_CatWander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Minimum wander distance from current location */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0"))
	float MinWanderRadius = 200.0f;

	/** Maximum wander distance from current location */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0"))
	float MaxWanderRadius = 800.0f;

	/** Acceptable distance to target to consider arrival */
	UPROPERTY(EditAnywhere, Category = "SmartCatAI", meta = (ClampMin = "0"))
	float AcceptanceRadius = 50.0f;

private:
	/** The target location we're moving to */
	FVector TargetLocation;

	/** Whether we successfully found a valid location */
	bool bHasValidTarget = false;
};
