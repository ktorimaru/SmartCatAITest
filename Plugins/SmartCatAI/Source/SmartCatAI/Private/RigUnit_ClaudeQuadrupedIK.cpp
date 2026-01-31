// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigUnit_ClaudeQuadrupedIK.h"
#include "Units/RigUnitContext.h"

FRigUnit_ClaudeQuadrupedIK_Execute()
{
	// Early out if disabled
	if (!bEnabled)
	{
		MasterAlpha = 0.0f;
		FrontLeftOutput.IKAlpha = 0.0f;
		FrontRightOutput.IKAlpha = 0.0f;
		BackLeftOutput.IKAlpha = 0.0f;
		BackRightOutput.IKAlpha = 0.0f;
		PelvisOffset = FVector::ZeroVector;
		PelvisRotation = FQuat::Identity;
		return;
	}

	MasterAlpha = 1.0f;

	// Get the hierarchy from ExecuteContext
	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (!Hierarchy)
	{
		return;
	}

	// Calculate speed and detect gait
	const float Speed = Velocity.Size2D();

	// Auto-detect gait based on speed
	if (Speed < WalkSpeed * 0.5f)
	{
		DetectedGait = EClaudeQuadrupedGait::Walk;
	}
	else if (Speed < TrotSpeed)
	{
		DetectedGait = EClaudeQuadrupedGait::Walk;
	}
	else if (Speed < GallopSpeed)
	{
		DetectedGait = EClaudeQuadrupedGait::Trot;
	}
	else
	{
		DetectedGait = EClaudeQuadrupedGait::Gallop;
	}

	// Use manual gait override if set, otherwise use detected
	const EClaudeQuadrupedGait ActiveGait = Gait;

	// Calculate gait cycle progression
	float StepsPerSecond = 0.0f;
	if (StrideLength > 0.0f && Speed > 1.0f)
	{
		StepsPerSecond = Speed / StrideLength;
	}

	// Accumulate phase
	if (bProceduralGait && Speed > 1.0f)
	{
		AccumulatedPhase += StepsPerSecond * DeltaTime;
		AccumulatedPhase = FMath::Fmod(AccumulatedPhase, 1.0f);
	}
	GaitCyclePhase = AccumulatedPhase;

	// Define leg phase offsets based on gait
	// Phase offset determines when each leg is in its swing phase relative to the cycle
	float PhaseOffset_FL = 0.0f;  // Front Left
	float PhaseOffset_FR = 0.0f;  // Front Right
	float PhaseOffset_BL = 0.0f;  // Back Left
	float PhaseOffset_BR = 0.0f;  // Back Right

	switch (ActiveGait)
	{
	case EClaudeQuadrupedGait::Walk:
		// 4-beat lateral sequence: LH(0) -> LF(0.25) -> RH(0.5) -> RF(0.75)
		// This creates the classic walking pattern
		PhaseOffset_FL = 0.25f;  // Front Left
		PhaseOffset_FR = 0.75f;  // Front Right
		PhaseOffset_BL = 0.0f;   // Back Left
		PhaseOffset_BR = 0.5f;   // Back Right
		break;

	case EClaudeQuadrupedGait::Trot:
		// 2-beat diagonal: LF+RB together, RF+LB together
		PhaseOffset_FL = 0.0f;   // Front Left
		PhaseOffset_FR = 0.5f;   // Front Right
		PhaseOffset_BL = 0.5f;   // Back Left
		PhaseOffset_BR = 0.0f;   // Back Right
		break;

	case EClaudeQuadrupedGait::Gallop:
		// Asymmetric bounding: Back legs lead, front legs follow
		// Back legs nearly together, front legs nearly together, with suspension
		PhaseOffset_FL = 0.55f;  // Front Left (slight offset from FR)
		PhaseOffset_FR = 0.45f;  // Front Right
		PhaseOffset_BL = 0.05f;  // Back Left (slight offset from BR)
		PhaseOffset_BR = 0.0f;   // Back Right
		break;
	}

	// Lambda to calculate step height based on phase
	// Returns 0 when grounded, peaks at 0.5 of swing phase
	auto CalculateStepCurve = [](float Phase, float SwingDuration) -> float
	{
		// Phase 0 to SwingDuration is swing (foot in air)
		// Phase SwingDuration to 1 is stance (foot on ground)
		if (Phase < SwingDuration)
		{
			// Swing phase - lift the foot
			float SwingProgress = Phase / SwingDuration;
			// Use sine curve for smooth lift and lower
			return FMath::Sin(SwingProgress * PI);
		}
		return 0.0f; // Stance phase - foot on ground
	};

	// Swing duration as fraction of gait cycle (how long foot is in air)
	float SwingDuration = 0.25f; // Default for walk
	switch (ActiveGait)
	{
	case EClaudeQuadrupedGait::Walk:
		SwingDuration = 0.25f;
		break;
	case EClaudeQuadrupedGait::Trot:
		SwingDuration = 0.4f;
		break;
	case EClaudeQuadrupedGait::Gallop:
		SwingDuration = 0.35f;
		break;
	}

	// Lambda to process a single leg
	auto ProcessLeg = [&](
		const FClaudeQuadrupedLegConfig& LegConfig,
		FClaudeQuadrupedLegOutput& Output,
		float PhaseOffset) -> float
	{
		if (!LegConfig.FootBone.IsValid())
		{
			Output.IKAlpha = 0.0f;
			return 0.0f;
		}

		const FTransform FootTransform = Hierarchy->GetGlobalTransform(LegConfig.FootBone);
		const FVector FootLocation = FootTransform.GetLocation();

		// Calculate this leg's phase in the gait cycle
		float LegPhase = FMath::Fmod(AccumulatedPhase + PhaseOffset, 1.0f);
		Output.StepPhase = LegPhase;
		Output.bIsSwinging = (LegPhase < SwingDuration);

		// Calculate ground position (simple ground plane)
		const float GroundZ = ComponentTransform.GetLocation().Z;

		// Calculate trace intersection
		const FVector TraceStart = FootLocation + FVector(0.0f, 0.0f, TraceStartOffset);
		const FVector TraceEnd = FootLocation - FVector(0.0f, 0.0f, TraceEndOffset);

		bool bHit = false;
		FVector HitPoint = FVector::ZeroVector;
		FVector HitNormal = FVector::UpVector;

		if (TraceStart.Z > GroundZ && TraceEnd.Z <= GroundZ)
		{
			float T = (TraceStart.Z - GroundZ) / (TraceStart.Z - TraceEnd.Z);
			if (T >= 0.0f && T <= 1.0f)
			{
				HitPoint = FMath::Lerp(TraceStart, TraceEnd, T);
				HitPoint.Z = GroundZ;
				bHit = true;
			}
		}

		float FootOffset = 0.0f;

		if (bHit)
		{
			Output.bHitGround = true;
			Output.GroundNormal = HitNormal;

			// Base target is ground position plus foot height
			FVector BaseTarget = HitPoint + FVector(0.0f, 0.0f, FootHeight);

			// Add procedural gait lift
			float LiftHeight = 0.0f;
			if (bProceduralGait && Speed > 1.0f)
			{
				LiftHeight = CalculateStepCurve(LegPhase, SwingDuration) * StepHeight;

				// Scale lift height with speed (faster = higher steps for gallop)
				if (ActiveGait == EClaudeQuadrupedGait::Gallop)
				{
					float SpeedFactor = FMath::Clamp(Speed / GallopSpeed, 0.5f, 1.5f);
					LiftHeight *= SpeedFactor;
				}
			}

			// Calculate forward/backward offset based on swing phase
			FVector SwingOffset = FVector::ZeroVector;
			if (bProceduralGait && Speed > 1.0f && Output.bIsSwinging)
			{
				// Move foot forward during swing
				FVector MoveDirection = Velocity.GetSafeNormal2D();
				float SwingProgress = LegPhase / SwingDuration;
				// Foot moves from back to front during swing
				float ForwardOffset = FMath::Lerp(-StrideLength * 0.3f, StrideLength * 0.3f, SwingProgress);
				SwingOffset = MoveDirection * ForwardOffset;
			}

			// Final IK target
			Output.IKTarget = BaseTarget + FVector(SwingOffset.X, SwingOffset.Y, LiftHeight);

			// Calculate foot offset for pelvis adjustment
			FootOffset = Output.IKTarget.Z - FootLocation.Z;
			FootOffset = FMath::Clamp(FootOffset, -MaxIKOffset, MaxIKOffset);

			// Foot rotation
			if (bAlignFootToGround && !Output.bIsSwinging)
			{
				if (FVector::DotProduct(HitNormal, FVector::UpVector) < 0.99f)
				{
					const FQuat AlignmentRotation = FQuat::FindBetweenNormals(FVector::UpVector, HitNormal);
					float Angle;
					FVector Axis;
					AlignmentRotation.ToAxisAndAngle(Axis, Angle);

					const float MaxAngleRadians = FMath::DegreesToRadians(MaxFootAngle);
					if (FMath::Abs(Angle) > MaxAngleRadians)
					{
						Angle = FMath::Sign(Angle) * MaxAngleRadians;
					}
					Output.FootRotation = FQuat(Axis, Angle) * FootTransform.GetRotation();
				}
				else
				{
					Output.FootRotation = FootTransform.GetRotation();
				}
			}
			else
			{
				Output.FootRotation = FootTransform.GetRotation();
			}

			Output.IKAlpha = 1.0f;
		}
		else
		{
			Output.bHitGround = false;
			Output.GroundNormal = FVector::UpVector;
			Output.IKTarget = FootLocation;
			Output.FootRotation = FootTransform.GetRotation();
			Output.IKAlpha = 0.0f;
		}

		return FootOffset;
	};

	// Process each leg with its phase offset
	float FootOffset_FrontLeft = ProcessLeg(FrontLeftLeg, FrontLeftOutput, PhaseOffset_FL);
	float FootOffset_FrontRight = ProcessLeg(FrontRightLeg, FrontRightOutput, PhaseOffset_FR);
	float FootOffset_BackLeft = ProcessLeg(BackLeftLeg, BackLeftOutput, PhaseOffset_BL);
	float FootOffset_BackRight = ProcessLeg(BackRightLeg, BackRightOutput, PhaseOffset_BR);

	// Calculate pelvis adjustment
	{
		const float MinOffset = FMath::Min(
			FMath::Min(FootOffset_FrontLeft, FootOffset_FrontRight),
			FMath::Min(FootOffset_BackLeft, FootOffset_BackRight)
		);

		if (MinOffset < 0.0f)
		{
			PelvisOffset = FVector(0.0f, 0.0f, MinOffset);
		}
		else
		{
			PelvisOffset = FVector::ZeroVector;
		}

		// Add vertical bob for gallop
		if (bProceduralGait && ActiveGait == EClaudeQuadrupedGait::Gallop && Speed > 1.0f)
		{
			// Suspension phase bob
			float BobPhase = FMath::Fmod(AccumulatedPhase * 2.0f, 1.0f);
			float Bob = FMath::Sin(BobPhase * PI * 2.0f) * StepHeight * 0.3f;
			PelvisOffset.Z += Bob;
		}

		// Calculate pelvis rotation
		const float FrontAvg = (FootOffset_FrontLeft + FootOffset_FrontRight) * 0.5f;
		const float BackAvg = (FootOffset_BackLeft + FootOffset_BackRight) * 0.5f;
		const float LeftAvg = (FootOffset_FrontLeft + FootOffset_BackLeft) * 0.5f;
		const float RightAvg = (FootOffset_FrontRight + FootOffset_BackRight) * 0.5f;

		const float PitchAngle = FMath::Atan2(FrontAvg - BackAvg, 100.0f);
		const float RollAngle = FMath::Atan2(RightAvg - LeftAvg, 50.0f);

		const float MaxTiltRadians = FMath::DegreesToRadians(15.0f);
		const float ClampedPitch = FMath::Clamp(PitchAngle, -MaxTiltRadians, MaxTiltRadians);
		const float ClampedRoll = FMath::Clamp(RollAngle, -MaxTiltRadians, MaxTiltRadians);

		PelvisRotation = FQuat(FRotator(
			FMath::RadiansToDegrees(ClampedPitch),
			0.0f,
			FMath::RadiansToDegrees(ClampedRoll)
		));
	}
}
