// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Units/RigUnit.h"
#include "RigUnit_ClaudeQuadrupedIK.generated.h"

/**
 * Gait type for quadruped locomotion
 */
UENUM(BlueprintType)
enum class EClaudeQuadrupedGait : uint8
{
	Walk    UMETA(DisplayName = "Walk"),      // 4-beat gait, each foot independent
	Trot    UMETA(DisplayName = "Trot"),      // 2-beat diagonal pairs
	Gallop  UMETA(DisplayName = "Gallop"),    // Asymmetric bounding gait
};

/**
 * Configuration for a single leg in the quadruped IK system
 */
USTRUCT(BlueprintType)
struct SMARTCATAI_API FClaudeQuadrupedLegConfig
{
	GENERATED_BODY()

	/** The IK target bone (foot/paw) */
	UPROPERTY(EditAnywhere, meta = (Input))
	FRigElementKey FootBone;

	/** The root bone of the IK chain (hip/shoulder) */
	UPROPERTY(EditAnywhere, meta = (Input))
	FRigElementKey IKRootBone;

	FClaudeQuadrupedLegConfig()
	{
	}
};

/**
 * Output data for a single leg
 */
USTRUCT(BlueprintType)
struct SMARTCATAI_API FClaudeQuadrupedLegOutput
{
	GENERATED_BODY()

	/** World space IK target position */
	UPROPERTY(EditAnywhere, meta = (Output))
	FVector IKTarget = FVector::ZeroVector;

	/** Foot rotation based on ground normal */
	UPROPERTY(EditAnywhere, meta = (Output))
	FQuat FootRotation = FQuat::Identity;

	/** IK blend weight (0 = animation, 1 = IK) */
	UPROPERTY(EditAnywhere, meta = (Output))
	float IKAlpha = 1.0f;

	/** Whether this foot hit ground */
	UPROPERTY(EditAnywhere, meta = (Output))
	bool bHitGround = false;

	/** The ground normal at the foot position */
	UPROPERTY(EditAnywhere, meta = (Output))
	FVector GroundNormal = FVector::UpVector;

	/** Current step phase (0-1, 0=grounded, 0.5=max lift) */
	UPROPERTY(EditAnywhere, meta = (Output))
	float StepPhase = 0.0f;

	/** Is this foot currently in swing phase (lifted) */
	UPROPERTY(EditAnywhere, meta = (Output))
	bool bIsSwinging = false;
};

/**
 * ClaudeQuadrupedIK - A Control Rig unit for procedural quadruped locomotion
 *
 * This node generates procedural walking/trotting/galloping animation for
 * quadruped characters with automatic foot placement and terrain adaptation.
 *
 * Supports three gaits:
 * - Walk: 4-beat lateral sequence gait
 * - Trot: 2-beat diagonal gait
 * - Gallop: Asymmetric bounding gait
 *
 * Created by Claude (Anthropic) for the SmartCatAI project.
 */
USTRUCT(meta = (DisplayName = "Claude Quadruped IK", Category = "SmartCatAI"))
struct SMARTCATAI_API FRigUnit_ClaudeQuadrupedIK : public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_ClaudeQuadrupedIK()
		: TraceStartOffset(50.0f)
		, TraceEndOffset(75.0f)
		, MaxIKOffset(30.0f)
		, FootHeight(2.0f)
		, bEnabled(true)
		, bProceduralGait(true)
		, Gait(EClaudeQuadrupedGait::Walk)
		, StrideLength(40.0f)
		, StepHeight(15.0f)
		, WalkSpeed(100.0f)
		, TrotSpeed(250.0f)
		, GallopSpeed(400.0f)
		, bAlignFootToGround(true)
		, MaxFootAngle(45.0f)
		, bDebugDraw(false)
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	// ============================================
	// Inputs - Character State
	// ============================================

	/** The component transform (character world transform) */
	UPROPERTY(EditAnywhere, meta = (Input))
	FTransform ComponentTransform;

	/** Character velocity in world space */
	UPROPERTY(EditAnywhere, meta = (Input))
	FVector Velocity = FVector::ZeroVector;

	/** Delta time for animation */
	UPROPERTY(EditAnywhere, meta = (Input))
	float DeltaTime = 0.0f;

	// ============================================
	// Inputs - Leg Configuration
	// ============================================

	/** Configuration for front left leg */
	UPROPERTY(EditAnywhere, meta = (Input))
	FClaudeQuadrupedLegConfig FrontLeftLeg;

	/** Configuration for front right leg */
	UPROPERTY(EditAnywhere, meta = (Input))
	FClaudeQuadrupedLegConfig FrontRightLeg;

	/** Configuration for back left leg */
	UPROPERTY(EditAnywhere, meta = (Input))
	FClaudeQuadrupedLegConfig BackLeftLeg;

	/** Configuration for back right leg */
	UPROPERTY(EditAnywhere, meta = (Input))
	FClaudeQuadrupedLegConfig BackRightLeg;

	/** Pelvis bone for body adjustment */
	UPROPERTY(EditAnywhere, meta = (Input))
	FRigElementKey PelvisBone;

	// ============================================
	// Inputs - Ground Adaptation
	// ============================================

	/** How far above the foot bone to start the trace */
	UPROPERTY(EditAnywhere, meta = (Input))
	float TraceStartOffset;

	/** How far below the foot bone to end the trace */
	UPROPERTY(EditAnywhere, meta = (Input))
	float TraceEndOffset;

	/** Maximum IK adjustment distance (prevents extreme stretching) */
	UPROPERTY(EditAnywhere, meta = (Input))
	float MaxIKOffset;

	/** Foot height offset from ground surface */
	UPROPERTY(EditAnywhere, meta = (Input))
	float FootHeight;

	// ============================================
	// Inputs - Gait Control
	// ============================================

	/** Whether the IK system is enabled */
	UPROPERTY(EditAnywhere, meta = (Input))
	bool bEnabled;

	/** Enable procedural gait generation (vs just ground adaptation) */
	UPROPERTY(EditAnywhere, meta = (Input))
	bool bProceduralGait;

	/** Current gait type */
	UPROPERTY(EditAnywhere, meta = (Input))
	EClaudeQuadrupedGait Gait;

	/** Distance covered per full step cycle */
	UPROPERTY(EditAnywhere, meta = (Input))
	float StrideLength;

	/** Maximum height of foot lift during swing phase */
	UPROPERTY(EditAnywhere, meta = (Input))
	float StepHeight;

	/** Speed threshold for walk gait */
	UPROPERTY(EditAnywhere, meta = (Input))
	float WalkSpeed;

	/** Speed threshold for trot gait */
	UPROPERTY(EditAnywhere, meta = (Input))
	float TrotSpeed;

	/** Speed threshold for gallop gait */
	UPROPERTY(EditAnywhere, meta = (Input))
	float GallopSpeed;

	/** Multiplier for gait speed (use >1 to speed up cycle for testing) */
	UPROPERTY(EditAnywhere, meta = (Input))
	float GaitSpeedMultiplier = 1.0f;

	// ============================================
	// Inputs - Foot Alignment
	// ============================================

	/** Whether to rotate feet to align with ground normal */
	UPROPERTY(EditAnywhere, meta = (Input))
	bool bAlignFootToGround;

	/** Maximum angle (degrees) for foot alignment to ground */
	UPROPERTY(EditAnywhere, meta = (Input))
	float MaxFootAngle;

	/** Enable debug drawing */
	UPROPERTY(EditAnywhere, meta = (Input))
	bool bDebugDraw;

	// ============================================
	// Outputs
	// ============================================

	/** Output data for front left leg */
	UPROPERTY(EditAnywhere, meta = (Output))
	FClaudeQuadrupedLegOutput FrontLeftOutput;

	/** Output data for front right leg */
	UPROPERTY(EditAnywhere, meta = (Output))
	FClaudeQuadrupedLegOutput FrontRightOutput;

	/** Output data for back left leg */
	UPROPERTY(EditAnywhere, meta = (Output))
	FClaudeQuadrupedLegOutput BackLeftOutput;

	/** Output data for back right leg */
	UPROPERTY(EditAnywhere, meta = (Output))
	FClaudeQuadrupedLegOutput BackRightOutput;

	/** Pelvis offset for body adjustment (local space) */
	UPROPERTY(EditAnywhere, meta = (Output))
	FVector PelvisOffset;

	/** Pelvis rotation adjustment */
	UPROPERTY(EditAnywhere, meta = (Output))
	FQuat PelvisRotation;

	/** Overall IK alpha (0 when disabled) */
	UPROPERTY(EditAnywhere, meta = (Output))
	float MasterAlpha;

	/** Current detected gait based on speed */
	UPROPERTY(EditAnywhere, meta = (Output))
	EClaudeQuadrupedGait DetectedGait;

	/** Accumulated gait cycle phase (0-1) */
	UPROPERTY(EditAnywhere, meta = (Output))
	float GaitCyclePhase;

	/** Debug: Current horizontal speed (Velocity.Size2D) */
	UPROPERTY(EditAnywhere, meta = (Output))
	float DebugSpeed = 0.0f;

	/** Debug: Steps per second calculation */
	UPROPERTY(EditAnywhere, meta = (Output))
	float DebugStepsPerSecond = 0.0f;

	/** Internal accumulated phase for gait cycle (persists between frames) */
	UPROPERTY(Transient, meta = (Input, Output))
	float AccumulatedPhase = 0.0f;
};
