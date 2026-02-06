// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SmartCatAnimInstance.h"
#include "SmartCatAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UAIPerceptionComponent;
class ASmartCatAICharacter;

/**
 * Cat mood states that influence behavior selection
 */
UENUM(BlueprintType)
enum class ECatMood : uint8
{
	Calm      UMETA(DisplayName = "Calm"),
	Alert     UMETA(DisplayName = "Alert"),
	Playful   UMETA(DisplayName = "Playful"),
	Tired     UMETA(DisplayName = "Tired"),
	Hungry    UMETA(DisplayName = "Hungry"),
	Scared    UMETA(DisplayName = "Scared"),
};

/**
 * High-level behavior categories
 */
UENUM(BlueprintType)
enum class ECatBehavior : uint8
{
	Idle      UMETA(DisplayName = "Idle"),
	Patrol    UMETA(DisplayName = "Patrol"),
	Hunt      UMETA(DisplayName = "Hunt"),
	Flee      UMETA(DisplayName = "Flee"),
	Play      UMETA(DisplayName = "Play"),
	Rest      UMETA(DisplayName = "Rest"),
	Groom     UMETA(DisplayName = "Groom"),
	Explore   UMETA(DisplayName = "Explore"),
};

/**
 * AI Controller for the SmartCat character
 * Manages autonomous cat behaviors using Behavior Trees
 */
UCLASS()
class SMARTCATAI_API ASmartCatAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASmartCatAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	// ============================================
	// High-Level Commands
	// ============================================

	/** Command the cat to move to a specific location */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Commands")
	void MoveToTarget(const FVector& Target);

	/** Trigger a specific behavior */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Commands")
	void TriggerBehavior(ECatBehavior Behavior);

	/** Set the cat's current mood */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Commands")
	void SetMood(ECatMood NewMood);

	/** Trigger an animation action */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Commands")
	void TriggerAction(ECatAnimationAction Action);

	/** Stop current behavior and return to idle */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Commands")
	void StopBehavior();

	// ============================================
	// State Queries
	// ============================================

	/** Get current mood */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|State")
	ECatMood GetCurrentMood() const { return CurrentMood; }

	/** Get current behavior */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|State")
	ECatBehavior GetCurrentBehavior() const { return CurrentBehavior; }

	/** Check if the cat is currently moving */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|State")
	bool IsMoving() const;

	/** Check if the cat is playing an action animation */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|State")
	bool IsPlayingAction() const;

	// ============================================
	// Perception Events
	// ============================================

	/** Called when an actor is perceived (sight, hearing, etc.) */
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, struct FAIStimulus Stimulus);

protected:
	// ============================================
	// Behavior Tree
	// ============================================

	/** Main behavior tree for autonomous cat AI */
	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|AI")
	TObjectPtr<UBehaviorTree> CatBehaviorTree;

	/** Whether to run the behavior tree on possess */
	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|AI")
	bool bAutoStartBehaviorTree = true;

	// ============================================
	// Perception
	// ============================================

	/** AI Perception component for sensing the environment */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SmartCatAI|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	// ============================================
	// State
	// ============================================

	/** Current mood affecting behavior selection */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|State")
	ECatMood CurrentMood = ECatMood::Calm;

	/** Current high-level behavior */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|State")
	ECatBehavior CurrentBehavior = ECatBehavior::Idle;

	/** Interest level in current target (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|State")
	float InterestLevel = 0.0f;

	// ============================================
	// Blackboard Keys
	// ============================================

	/** Blackboard key names for easy reference */
	static const FName BB_MoveTarget;
	static const FName BB_LookTarget;
	static const FName BB_CurrentMood;
	static const FName BB_CurrentBehavior;
	static const FName BB_InterestLevel;
	static const FName BB_CurrentAction;

private:
	/** Cached reference to the controlled cat character */
	UPROPERTY()
	TObjectPtr<ASmartCatAICharacter> CatCharacter;

	/** Update blackboard with current state */
	void UpdateBlackboard();
};
