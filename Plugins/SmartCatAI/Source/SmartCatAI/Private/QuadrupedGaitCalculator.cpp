// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuadrupedGaitCalculator.h"

void UQuadrupedGaitCalculator::UpdateGaitState(
	FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& Velocity,
	float DeltaTime)
{
	const float Speed = Velocity.Size2D();
	State.DebugSpeed = Speed;

	// Auto-detect gait based on speed
	if (Speed < Config.StrollSpeed)
	{
		State.DetectedGait = EQuadrupedGait::Stroll;
	}
	else if (Speed < Config.WalkSpeed)
	{
		State.DetectedGait = EQuadrupedGait::Walk;
	}
	else if (Speed < Config.TrotSpeed)
	{
		State.DetectedGait = EQuadrupedGait::Trot;
	}
	else
	{
		State.DetectedGait = EQuadrupedGait::Gallop;
	}

	// Calculate steps per second
	float StepsPerSecond = 0.0f;
	if (Config.StrideLength > 0.0f && Speed > 0.1f)
	{
		StepsPerSecond = Speed / Config.StrideLength;
	}

	// Accumulate phase
	if (Config.bProceduralGait && Speed > 0.1f)
	{
		float EffectiveMultiplier = FMath::Max(Config.GaitSpeedMultiplier, 0.1f);
		State.AccumulatedPhase += StepsPerSecond * DeltaTime * EffectiveMultiplier;
		State.AccumulatedPhase = FMath::Fmod(State.AccumulatedPhase, 1.0f);
	}

	State.GaitCyclePhase = State.AccumulatedPhase;
}

void UQuadrupedGaitCalculator::GetPhaseOffsets(EQuadrupedGait Gait, float& OutFL, float& OutFR, float& OutBL, float& OutBR)
{
	switch (Gait)
	{
	case EQuadrupedGait::Stroll:
	case EQuadrupedGait::Walk:
		// 4-beat lateral sequence
		OutFL = 0.25f;
		OutFR = 0.75f;
		OutBL = 0.0f;
		OutBR = 0.5f;
		break;

	case EQuadrupedGait::Trot:
		// 2-beat diagonal pairs
		OutFL = 0.0f;
		OutFR = 0.5f;
		OutBL = 0.5f;
		OutBR = 0.0f;
		break;

	case EQuadrupedGait::Gallop:
		// Asymmetric bounding
		OutFL = 0.55f;
		OutFR = 0.45f;
		OutBL = 0.05f;
		OutBR = 0.0f;
		break;

	default:
		OutFL = 0.25f;
		OutFR = 0.75f;
		OutBL = 0.0f;
		OutBR = 0.5f;
		break;
	}
}

float UQuadrupedGaitCalculator::GetSwingDuration(EQuadrupedGait Gait)
{
	switch (Gait)
	{
	case EQuadrupedGait::Stroll:
		return 0.2f;
	case EQuadrupedGait::Walk:
		return 0.25f;
	case EQuadrupedGait::Trot:
		return 0.4f;
	case EQuadrupedGait::Gallop:
		return 0.35f;
	default:
		return 0.25f;
	}
}

float UQuadrupedGaitCalculator::CalculateStepCurve(float Phase, float SwingDuration)
{
	if (Phase < SwingDuration)
	{
		float SwingProgress = Phase / SwingDuration;
		return FMath::Sin(SwingProgress * PI);
	}
	return 0.0f;
}

FQuadrupedLegGaitOutput UQuadrupedGaitCalculator::CalculateLegOutput(
	const FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& MoveDirection,
	float PhaseOffset,
	float SwingDuration)
{
	FQuadrupedLegGaitOutput Output;

	const float Speed = State.DebugSpeed;
	const EQuadrupedGait ActiveGait = Config.bAutoGait ? State.DetectedGait : Config.ManualGait;

	// Calculate leg phase
	float LegPhase = FMath::Fmod(State.AccumulatedPhase + PhaseOffset, 1.0f);
	Output.StepPhase = LegPhase;
	Output.bIsSwinging = (LegPhase < SwingDuration);

	if (!Config.bProceduralGait || Speed <= 0.1f)
	{
		return Output;
	}

	// Calculate lift height
	Output.LiftHeight = CalculateStepCurve(LegPhase, SwingDuration) * Config.StepHeight;

	// Scale lift height for gallop
	if (ActiveGait == EQuadrupedGait::Gallop)
	{
		float SpeedFactor = FMath::Clamp(Speed / Config.GallopSpeed, 0.5f, 1.5f);
		Output.LiftHeight *= SpeedFactor;
	}

	// Calculate stride offset (forward/backward)
	float HalfStride = Config.StrideLength * 0.5f;

	if (Output.bIsSwinging)
	{
		// Swing phase: foot moves from back to front
		float SwingProgress = LegPhase / SwingDuration;
		Output.StrideOffset = FMath::Lerp(-HalfStride, HalfStride, SwingProgress);
	}
	else
	{
		// Stance phase: foot slides back
		float StanceProgress = (LegPhase - SwingDuration) / (1.0f - SwingDuration);
		Output.StrideOffset = FMath::Lerp(HalfStride, -HalfStride, StanceProgress);
	}

	// Build final position offset
	Output.PositionOffset = MoveDirection * Output.StrideOffset + FVector(0.0f, 0.0f, Output.LiftHeight);

	return Output;
}

FQuadrupedLegGaitOutput UQuadrupedGaitCalculator::CalculateFrontLeftLeg(
	const FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& MoveDirection)
{
	const EQuadrupedGait ActiveGait = Config.bAutoGait ? State.DetectedGait : Config.ManualGait;
	float FL, FR, BL, BR;
	GetPhaseOffsets(ActiveGait, FL, FR, BL, BR);
	float SwingDuration = GetSwingDuration(ActiveGait);
	return CalculateLegOutput(State, Config, MoveDirection, FL, SwingDuration);
}

FQuadrupedLegGaitOutput UQuadrupedGaitCalculator::CalculateFrontRightLeg(
	const FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& MoveDirection)
{
	const EQuadrupedGait ActiveGait = Config.bAutoGait ? State.DetectedGait : Config.ManualGait;
	float FL, FR, BL, BR;
	GetPhaseOffsets(ActiveGait, FL, FR, BL, BR);
	float SwingDuration = GetSwingDuration(ActiveGait);
	return CalculateLegOutput(State, Config, MoveDirection, FR, SwingDuration);
}

FQuadrupedLegGaitOutput UQuadrupedGaitCalculator::CalculateBackLeftLeg(
	const FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& MoveDirection)
{
	const EQuadrupedGait ActiveGait = Config.bAutoGait ? State.DetectedGait : Config.ManualGait;
	float FL, FR, BL, BR;
	GetPhaseOffsets(ActiveGait, FL, FR, BL, BR);
	float SwingDuration = GetSwingDuration(ActiveGait);
	return CalculateLegOutput(State, Config, MoveDirection, BL, SwingDuration);
}

FQuadrupedLegGaitOutput UQuadrupedGaitCalculator::CalculateBackRightLeg(
	const FQuadrupedGaitState& State,
	const FQuadrupedGaitConfig& Config,
	const FVector& MoveDirection)
{
	const EQuadrupedGait ActiveGait = Config.bAutoGait ? State.DetectedGait : Config.ManualGait;
	float FL, FR, BL, BR;
	GetPhaseOffsets(ActiveGait, FL, FR, BL, BR);
	float SwingDuration = GetSwingDuration(ActiveGait);
	return CalculateLegOutput(State, Config, MoveDirection, BR, SwingDuration);
}
