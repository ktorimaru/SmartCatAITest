// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "QuadrupedGaitCalculator.h"
#include "SmartCatAnimInstance.generated.h"

class ASmartCatAICharacter;

/**
 * Animation action types that can be triggered
 */
UENUM(BlueprintType)
enum class ECatAnimationAction : uint8
{
	None     UMETA(DisplayName = "None"),
	Flip     UMETA(DisplayName = "Flip"),
	Attack   UMETA(DisplayName = "Attack"),
	Fall     UMETA(DisplayName = "Fall"),
	Hear     UMETA(DisplayName = "Hear"),
	Focus    UMETA(DisplayName = "Focus"),
	LayDown  UMETA(DisplayName = "Lay Down"),
	Sit      UMETA(DisplayName = "Sit"),
	Sleep    UMETA(DisplayName = "Sleep"),
	Jump     UMETA(DisplayName = "Jump"),
	Land     UMETA(DisplayName = "Land"),
	Lick     UMETA(DisplayName = "Lick"),
	Meow     UMETA(DisplayName = "Meow"),
	Stretch  UMETA(DisplayName = "Stretch"),
};

/**
 * IK mode for the cat animation system
 */
UENUM(BlueprintType)
enum class ECatIKMode : uint8
{
	/** IK disabled - animation plays as-is */
	Disabled UMETA(DisplayName = "Disabled"),

	/** Slope adaptation - rotates mesh to match terrain slope, minimal per-foot IK */
	SlopeAdaptation UMETA(DisplayName = "Slope Adaptation"),

	/** Terrain adaptation only - adjusts feet to ground, no procedural gait */
	TerrainAdaptation UMETA(DisplayName = "Terrain Adaptation"),

	/** Full procedural - gait calculator + terrain adaptation (testing/special cases) */
	FullProcedural UMETA(DisplayName = "Full Procedural"),
};

UCLASS()
class SMARTCATAI_API USmartCatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	USmartCatAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Animation")
	ASmartCatAICharacter* GetSmartCatCharacter() const { return CatCharacter; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Animation")
	TObjectPtr<ASmartCatAICharacter> CatCharacter;

	// Movement state for blend spaces
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	FVector Velocity;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	bool bIsMoving;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	bool bIsFalling;

	/** Normalized movement direction (2D) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Movement")
	FVector MoveDirection;

	// ============================================
	// Animation Actions
	// ============================================

	/** Current requested animation action */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Animation")
	ECatAnimationAction CurrentAction = ECatAnimationAction::None;

	/** Whether an action animation is currently playing */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Animation")
	bool bIsPlayingAction = false;

public:
	/** Request an animation action to play */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Animation")
	void TriggerAction(ECatAnimationAction Action);

	/** Clear the current action (call when animation finishes) */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Animation")
	void ClearAction();

	/** Check if an action animation is currently playing */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|Animation")
	bool IsPlayingAction() const { return bIsPlayingAction; }

	/** Get current animation action */
	UFUNCTION(BlueprintPure, Category = "SmartCatAI|Animation")
	ECatAnimationAction GetCurrentAction() const { return CurrentAction; }

	/**
	 * Debug: Export gait data to CSV file for analysis
	 * Outputs phase, swing status, lift height for each leg across speed range
	 * @param MinSpeed - Starting speed for analysis
	 * @param MaxSpeed - Ending speed for analysis
	 * @param SpeedStep - Increment between speed samples
	 * @param TimeStep - Delta time for simulation (default 0.016 = 60fps)
	 * @param CycleDuration - How long to simulate each speed (seconds)
	 */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Debug")
	void ExportGaitDataToCSV(float MinSpeed = 0.0f, float MaxSpeed = 500.0f, float SpeedStep = 25.0f, float TimeStep = 0.016f, float CycleDuration = 2.0f);

	/** Debug: Start recording real-time IK data during gameplay */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Debug")
	void StartRuntimeDebugRecording();

	/** Debug: Stop recording and save to file */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Debug")
	void StopRuntimeDebugRecording();

	/** Debug: Print current IK state to screen (call every frame to see live values) */
	UFUNCTION(BlueprintCallable, Category = "SmartCatAI|Debug")
	void PrintDebugState();

protected:
	/** Whether we're recording debug data */
	bool bIsRecordingDebug = false;

	/** Accumulated debug data */
	FString DebugRecordingData;

protected:
	// ============================================
	// IK Mode
	// ============================================

	/** Current IK mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK")
	ECatIKMode IKMode = ECatIKMode::SlopeAdaptation;

	// ============================================
	// Terrain Adaptation IK Data (for Animation Blueprint)
	// These are OFFSETS from the animated bone position
	// ============================================

	/** Z offset for front left foot (positive = raise, negative = lower) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float FootOffset_FL;

	/** Z offset for front right foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float FootOffset_FR;

	/** Z offset for back left foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float FootOffset_BL;

	/** Z offset for back right foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float FootOffset_BR;

	/** Ground normal at front left foot (for foot rotation) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FVector GroundNormal_FL;

	/** Ground normal at front right foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FVector GroundNormal_FR;

	/** Ground normal at back left foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FVector GroundNormal_BL;

	/** Ground normal at back right foot */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FVector GroundNormal_BR;

	/** Foot rotation to align with ground normal - Front Left */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FRotator FootRotation_FL;

	/** Foot rotation to align with ground normal - Front Right */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FRotator FootRotation_FR;

	/** Foot rotation to align with ground normal - Back Left */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FRotator FootRotation_BL;

	/** Foot rotation to align with ground normal - Back Right */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FRotator FootRotation_BR;

	// ============================================
	// Pelvis Adjustment (for Animation Blueprint)
	// ============================================

	/** Pelvis Z offset (negative = lower body) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float PelvisOffsetZ;

	/** Pelvis pitch from slope (positive = nose up) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float PelvisPitch;

	/** Pelvis roll from side slope (positive = right side up) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	float PelvisRoll;

	/** Combined pelvis adjustment as a rotator (for Transform Bone node) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|TerrainAdaptation")
	FRotator PelvisRotation;

	// ============================================
	// Slope Adaptation Data (for Animation Blueprint)
	// Primary terrain adaptation via mesh rotation
	// ============================================

	/** Ground Z height at front left paw */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float GroundZ_FL;

	/** Ground Z height at front right paw */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float GroundZ_FR;

	/** Ground Z height at back left paw */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float GroundZ_BL;

	/** Ground Z height at back right paw */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float GroundZ_BR;

	/** Calculated slope pitch in degrees (positive = climbing/nose up, negative = descending/nose down) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float SlopePitch;

	/** Calculated slope roll in degrees (positive = left side higher, negative = right side higher) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float SlopeRoll;

	/** Combined slope rotation to apply to mesh/root bone */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	FRotator SlopeRotation;

	/** Average ground height across all four paws */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float AverageGroundZ;

	/** Residual foot offset after slope rotation (for per-foot IK fine-tuning) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float ResidualOffset_FL;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float ResidualOffset_FR;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float ResidualOffset_BL;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Slope")
	float ResidualOffset_BR;

	// ============================================
	// Legacy IK foot effector targets (world space)
	// Used for FullProcedural mode
	// ============================================

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FVector IKFootTarget_FrontLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FVector IKFootTarget_FrontRight;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FVector IKFootTarget_BackLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FVector IKFootTarget_BackRight;

	// IK foot effector transforms (location + rotation for FABRIK)
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FTransform IKFootTransform_FrontLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FTransform IKFootTransform_FrontRight;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FTransform IKFootTransform_BackLeft;

	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK|Procedural")
	FTransform IKFootTransform_BackRight;

	// ============================================
	// IK Blend Weights
	// ============================================

	// IK blend weights (0 = animation, 1 = IK target)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_FrontLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_FrontRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_BackLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IKAlpha_BackRight;

	/** Overall IK alpha for all feet */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	float IKAlpha;

	// Pelvis offset for body adjustment (legacy - use PelvisOffsetZ instead)
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|IK")
	FVector PelvisOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PelvisAlpha;

	// ============================================
	// IK Configuration
	// ============================================

	/** Bone names for foot placement tracing */
	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_FrontLeft = TEXT("Cat-Shorthair-L-Hand");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_FrontRight = TEXT("Cat-Shorthair-R-Hand");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_BackLeft = TEXT("Cat-Shorthair-L-Toe0");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_BackRight = TEXT("Cat-Shorthair-R-Toe0");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_Pelvis = TEXT("Cat-Shorthair-Pelvis");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_Bell = TEXT("Cat-Shorthair-Bell");

	UPROPERTY(EditDefaultsOnly, Category = "SmartCatAI|IK|Config")
	FName BoneName_Jaw = TEXT("Cat-Shorthair-Jaw");

	/** How far above the foot bone to start the trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float TraceStartOffset = 50.0f;

	/** How far below the foot bone to end the trace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float TraceEndOffset = 75.0f;

	/** Collision channel for foot traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Maximum IK adjustment distance (prevents extreme stretching) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float MaxIKOffset = 30.0f;

	/** Speed of IK target interpolation (higher = faster response) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float IKInterpSpeed = 15.0f;

	/** Speed of pelvis offset interpolation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float PelvisInterpSpeed = 10.0f;

	/** Minimum ground speed to enable IK (disables during fast movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float IKDisableSpeedThreshold = 400.0f;

	/** Foot height offset from ground surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float FootHeight = 2.0f;

	/** Height threshold above ground to consider foot in swing phase (reduces IK) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float SwingPhaseHeightThreshold = 5.0f;

	/** How quickly per-foot IK alpha blends (higher = faster) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float FootIKBlendSpeed = 15.0f;

	/** Approximate body length (front to back paw distance) for slope angle calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float BodyLength = 60.0f;

	/** Approximate body width (left to right paw distance) for roll angle calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float BodyWidth = 20.0f;

	/** Speed of slope rotation interpolation (higher = faster response to terrain) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float SlopeInterpSpeed = 8.0f;

	/** Maximum slope pitch angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float MaxSlopePitch = 30.0f;

	/** Maximum slope roll angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float MaxSlopeRoll = 15.0f;

	/** Threshold for residual offset to apply per-foot IK (below this, skip foot IK) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Config")
	float ResidualIKThreshold = 3.0f;

	// ============================================
	// IK Debug
	// ============================================

	/** Enable debug visualization of IK traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|IK|Debug")
	bool bDrawDebugTraces = false;

	// ============================================
	// Gait Configuration
	// ============================================

	/** Gait configuration for procedural animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SmartCatAI|Gait")
	FQuadrupedGaitConfig GaitConfig;

	/** Current gait state (read-only, updated automatically) */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Gait")
	FQuadrupedGaitState GaitState;

	/** Current detected gait for display */
	UPROPERTY(BlueprintReadOnly, Category = "SmartCatAI|Gait")
	EQuadrupedGait CurrentGait;

protected:
	void UpdateMovementState(float DeltaSeconds);
	void UpdateIKTargets(float DeltaSeconds);
	void UpdateGait(float DeltaSeconds);

	/** Update slope adaptation IK (Mode: SlopeAdaptation) - rotates mesh to match terrain */
	void UpdateSlopeAdaptationIK(float DeltaSeconds);

	/** Update terrain adaptation IK (Mode: TerrainAdaptation) */
	void UpdateTerrainAdaptationIK(float DeltaSeconds);

	/** Update procedural gait IK (Mode: FullProcedural) */
	void UpdateProceduralIK(float DeltaSeconds);

private:
	/** Perform a single foot trace and return the hit location */
	bool TraceFootToGround(const FName& BoneName, FVector& OutHitLocation, FVector& OutHitNormal);

	/** Calculate the foot offset needed based on trace result */
	float CalculateFootOffset(const FVector& TraceHitLocation, const FVector& BoneWorldLocation);

	/** Calculate pelvis offset based on all foot offsets */
	FVector CalculatePelvisOffset();

	/** Calculate pelvis pitch and roll from foot heights */
	void CalculatePelvisRotation();

	/** Calculate foot rotation from ground normal */
	FRotator CalculateFootRotationFromNormal(const FVector& GroundNormal) const;

	/** Smoothly interpolate a foot target */
	FVector InterpFootTarget(const FVector& Current, const FVector& Target, float DeltaSeconds);

	/** Check if IK should be active based on movement state */
	bool ShouldEnableIK() const;

	/** Get the effective IK mode (may override based on state) */
	ECatIKMode GetEffectiveIKMode() const;

	// ============================================
	// Internal trace data
	// ============================================

	/** Raw trace hit locations before interpolation */
	FVector RawFootLocation_FrontLeft;
	FVector RawFootLocation_FrontRight;
	FVector RawFootLocation_BackLeft;
	FVector RawFootLocation_BackRight;

	/** Raw foot Z offsets (before interpolation) */
	float RawFootOffset_FL = 0.0f;
	float RawFootOffset_FR = 0.0f;
	float RawFootOffset_BL = 0.0f;
	float RawFootOffset_BR = 0.0f;

	/** Legacy foot offsets (for procedural mode) */
	float FootOffset_FrontLeft = 0.0f;
	float FootOffset_FrontRight = 0.0f;
	float FootOffset_BackLeft = 0.0f;
	float FootOffset_BackRight = 0.0f;

	/** Cached reference to skeletal mesh component */
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	/** Whether IK is currently enabled */
	bool bIKEnabled = false;
};
