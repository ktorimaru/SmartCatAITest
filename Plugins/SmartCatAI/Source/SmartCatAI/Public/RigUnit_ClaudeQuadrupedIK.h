// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Units/RigUnit.h"
#include "RigUnit_ClaudeQuadrupedIK.generated.h"

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
};

/**
 * ClaudeQuadrupedIK - A Control Rig unit for procedural quadruped foot placement
 *
 * This node performs ground traces for all four legs of a quadruped character
 * and calculates IK targets, foot rotations, and pelvis adjustments for
 * realistic foot placement on uneven terrain.
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
		, bAlignFootToGround(true)
		, MaxFootAngle(45.0f)
		, bDebugDraw(false)
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	// ============================================
	// Inputs
	// ============================================

	/** The component transform (character world transform) */
	UPROPERTY(EditAnywhere, meta = (Input))
	FTransform ComponentTransform;

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

	/** Whether the IK system is enabled */
	UPROPERTY(EditAnywhere, meta = (Input))
	bool bEnabled;

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
};
