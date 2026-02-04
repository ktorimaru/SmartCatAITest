// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuadrupedGaitCalculator.generated.h"

/**
 * Gait type for quadruped locomotion
 */
UENUM(BlueprintType)
enum class EQuadrupedGait : uint8
{
	Stroll  UMETA(DisplayName = "Stroll"),
	Walk    UMETA(DisplayName = "Walk"),
	Trot    UMETA(DisplayName = "Trot"),
	Gallop  UMETA(DisplayName = "Gallop"),
};

/**
 * Configuration for quadruped gait calculations
 */
USTRUCT(BlueprintType)
struct SMARTCATAI_API FQuadrupedGaitConfig
{
	GENERATED_BODY()

	/** Distance covered per full step cycle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrideLength = 40.0f;

	/** Maximum height of foot lift during swing phase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StepHeight = 15.0f;

	/** Speed threshold: below this is Stroll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrollSpeed = 75.0f;

	/** Speed threshold: above StrollSpeed, below this is Walk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WalkSpeed = 110.0f;

	/** Speed threshold: above WalkSpeed, below this is Trot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TrotSpeed = 145.0f;

	/** Speed threshold: above TrotSpeed is Gallop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GallopSpeed = 145.0f;

	/** Multiplier for gait speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GaitSpeedMultiplier = 1.0f;

	/** Enable procedural gait generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bProceduralGait = true;

	/** Automatically switch gait based on speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoGait = true;

	/** Manual gait selection (used when bAutoGait is false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EQuadrupedGait ManualGait = EQuadrupedGait::Walk;
};

/**
 * Output for a single leg's gait calculation
 */
USTRUCT(BlueprintType)
struct SMARTCATAI_API FQuadrupedLegGaitOutput
{
	GENERATED_BODY()

	/** Offset to add to base foot position (world space) */
	UPROPERTY(BlueprintReadOnly)
	FVector PositionOffset = FVector::ZeroVector;

	/** Effector rotation (for IK) - toe points in movement direction, pitched based on swing phase */
	UPROPERTY(BlueprintReadOnly)
	FRotator EffectorRotation = FRotator::ZeroRotator;

	/** Full effector transform (position offset + rotation) */
	UPROPERTY(BlueprintReadOnly)
	FTransform EffectorTransform = FTransform::Identity;

	/** Foot lift height */
	UPROPERTY(BlueprintReadOnly)
	float LiftHeight = 0.0f;

	/** Forward/backward offset along movement direction */
	UPROPERTY(BlueprintReadOnly)
	float StrideOffset = 0.0f;

	/** Current step phase (0-1) */
	UPROPERTY(BlueprintReadOnly)
	float StepPhase = 0.0f;

	/** Is this foot in swing phase (lifted) */
	UPROPERTY(BlueprintReadOnly)
	bool bIsSwinging = false;

	/** Swing progress (0-1) during swing phase, 0 during stance */
	UPROPERTY(BlueprintReadOnly)
	float SwingProgress = 0.0f;
};

/**
 * Persistent state for gait calculation (store in your class)
 */
USTRUCT(BlueprintType)
struct SMARTCATAI_API FQuadrupedGaitState
{
	GENERATED_BODY()

	/** Accumulated phase (0-1, wraps) */
	UPROPERTY()
	float AccumulatedPhase = 0.0f;

	/** Current detected gait */
	UPROPERTY(BlueprintReadOnly)
	EQuadrupedGait DetectedGait = EQuadrupedGait::Walk;

	/** Current gait cycle phase */
	UPROPERTY(BlueprintReadOnly)
	float GaitCyclePhase = 0.0f;

	/** Debug: current speed */
	UPROPERTY(BlueprintReadOnly)
	float DebugSpeed = 0.0f;
};

/**
 * Static utility class for quadruped gait calculations
 * Can be used by both AnimInstance and Control Rig
 */
UCLASS()
class SMARTCATAI_API UQuadrupedGaitCalculator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Update the gait state based on current velocity and delta time
	 * Call this once per frame before calculating individual leg outputs
	 */
	static void UpdateGaitState(
		FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& Velocity,
		float DeltaTime
	);

	/**
	 * Calculate gait output for front left leg
	 */
	static FQuadrupedLegGaitOutput CalculateFrontLeftLeg(
		const FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& MoveDirection
	);

	/**
	 * Calculate gait output for front right leg
	 */
	static FQuadrupedLegGaitOutput CalculateFrontRightLeg(
		const FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& MoveDirection
	);

	/**
	 * Calculate gait output for back left leg
	 */
	static FQuadrupedLegGaitOutput CalculateBackLeftLeg(
		const FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& MoveDirection
	);

	/**
	 * Calculate gait output for back right leg
	 */
	static FQuadrupedLegGaitOutput CalculateBackRightLeg(
		const FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& MoveDirection
	);

private:
	/** Get phase offsets for the given gait */
	static void GetPhaseOffsets(EQuadrupedGait Gait, float& OutFL, float& OutFR, float& OutBL, float& OutBR);

	/** Get swing duration for the given gait */
	static float GetSwingDuration(EQuadrupedGait Gait);

	/** Calculate step curve (0 when grounded, peaks at 0.5 of swing) */
	static float CalculateStepCurve(float Phase, float SwingDuration);

	/** Calculate output for a single leg given its phase offset */
	static FQuadrupedLegGaitOutput CalculateLegOutput(
		const FQuadrupedGaitState& State,
		const FQuadrupedGaitConfig& Config,
		const FVector& MoveDirection,
		float PhaseOffset,
		float SwingDuration
	);
};
