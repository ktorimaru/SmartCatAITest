// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartCatAIController.h"
#include "SmartCatAICharacter.h"
#include "SmartCatAnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Navigation/PathFollowingComponent.h"

// Blackboard key names
const FName ASmartCatAIController::BB_MoveTarget = TEXT("MoveTarget");
const FName ASmartCatAIController::BB_LookTarget = TEXT("LookTarget");
const FName ASmartCatAIController::BB_CurrentMood = TEXT("CurrentMood");
const FName ASmartCatAIController::BB_CurrentBehavior = TEXT("CurrentBehavior");
const FName ASmartCatAIController::BB_InterestLevel = TEXT("InterestLevel");
const FName ASmartCatAIController::BB_CurrentAction = TEXT("CurrentAction");

ASmartCatAIController::ASmartCatAIController()
	: CatCharacter(nullptr)
{
	// Create perception component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// Configure sight sense
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.0f;
	SightConfig->LoseSightRadius = 1200.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*SightConfig);

	// Configure hearing sense
	UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 800.0f;
	HearingConfig->SetMaxAge(3.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*HearingConfig);

	// Set dominant sense
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind perception update delegate
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ASmartCatAIController::OnTargetPerceptionUpdated);
}

void ASmartCatAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ASmartCatAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Cache cat character reference
	CatCharacter = Cast<ASmartCatAICharacter>(InPawn);

	// Start behavior tree if configured
	if (bAutoStartBehaviorTree && CatBehaviorTree)
	{
		RunBehaviorTree(CatBehaviorTree);
		UpdateBlackboard();
	}
}

void ASmartCatAIController::OnUnPossess()
{
	// Stop behavior tree
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("UnPossess"));
	}

	CatCharacter = nullptr;

	Super::OnUnPossess();
}

void ASmartCatAIController::MoveToTarget(const FVector& Target)
{
	if (!GetPawn())
	{
		return;
	}

	// Update blackboard
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(BB_MoveTarget, Target);
	}

	// Use the built-in MoveToLocation
	MoveToLocation(Target);
}

void ASmartCatAIController::TriggerBehavior(ECatBehavior Behavior)
{
	CurrentBehavior = Behavior;
	UpdateBlackboard();

	UE_LOG(LogTemp, Log, TEXT("SmartCatAI: Behavior changed to %d"), static_cast<int32>(Behavior));
}

void ASmartCatAIController::SetMood(ECatMood NewMood)
{
	if (CurrentMood != NewMood)
	{
		CurrentMood = NewMood;
		UpdateBlackboard();

		UE_LOG(LogTemp, Log, TEXT("SmartCatAI: Mood changed to %d"), static_cast<int32>(NewMood));
	}
}

void ASmartCatAIController::TriggerAction(ECatAnimationAction Action)
{
	if (CatCharacter)
	{
		if (USmartCatAnimInstance* AnimInstance = Cast<USmartCatAnimInstance>(CatCharacter->GetMesh()->GetAnimInstance()))
		{
			AnimInstance->TriggerAction(Action);
		}
	}

	// Update blackboard
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(BB_CurrentAction, static_cast<uint8>(Action));
	}
}

void ASmartCatAIController::StopBehavior()
{
	// Stop movement
	StopMovement();

	// Reset to idle behavior
	CurrentBehavior = ECatBehavior::Idle;
	UpdateBlackboard();

	// Clear any current action
	if (CatCharacter)
	{
		if (USmartCatAnimInstance* AnimInstance = Cast<USmartCatAnimInstance>(CatCharacter->GetMesh()->GetAnimInstance()))
		{
			AnimInstance->ClearAction();
		}
	}
}

bool ASmartCatAIController::IsMoving() const
{
	if (const UPathFollowingComponent* PathComp = GetPathFollowingComponent())
	{
		return PathComp->GetStatus() == EPathFollowingStatus::Moving;
	}
	return false;
}

bool ASmartCatAIController::IsPlayingAction() const
{
	if (CatCharacter)
	{
		if (const USmartCatAnimInstance* AnimInstance = Cast<USmartCatAnimInstance>(CatCharacter->GetMesh()->GetAnimInstance()))
		{
			return AnimInstance->IsPlayingAction();
		}
	}
	return false;
}

void ASmartCatAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor == GetPawn())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Something was sensed
		UE_LOG(LogTemp, Log, TEXT("SmartCatAI: Sensed actor %s (Strength: %.2f)"),
			*Actor->GetName(), Stimulus.Strength);

		// Increase interest level
		InterestLevel = FMath::Clamp(InterestLevel + 0.2f, 0.0f, 1.0f);

		// Update blackboard with look target
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsObject(BB_LookTarget, Actor);
			BB->SetValueAsFloat(BB_InterestLevel, InterestLevel);
		}

		// If hearing and calm, become alert
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && CurrentMood == ECatMood::Calm)
		{
			SetMood(ECatMood::Alert);
			TriggerAction(ECatAnimationAction::Hear);
		}
	}
	else
	{
		// Lost sight/hearing of something
		InterestLevel = FMath::Clamp(InterestLevel - 0.1f, 0.0f, 1.0f);

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsFloat(BB_InterestLevel, InterestLevel);
		}
	}
}

void ASmartCatAIController::UpdateBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	BB->SetValueAsEnum(BB_CurrentMood, static_cast<uint8>(CurrentMood));
	BB->SetValueAsEnum(BB_CurrentBehavior, static_cast<uint8>(CurrentBehavior));
	BB->SetValueAsFloat(BB_InterestLevel, InterestLevel);
}
