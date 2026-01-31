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

	// Lambda to process a single leg
	auto ProcessLeg = [&](const FClaudeQuadrupedLegConfig& LegConfig, FClaudeQuadrupedLegOutput& Output) -> float
	{
		if (!LegConfig.FootBone.IsValid())
		{
			Output.IKAlpha = 0.0f;
			return 0.0f;
		}

		const FTransform FootTransform = Hierarchy->GetGlobalTransform(LegConfig.FootBone);
		const FVector FootLocation = FootTransform.GetLocation();

		// Calculate trace start and end in world space
		const FVector TraceStart = FootLocation + FVector(0.0f, 0.0f, TraceStartOffset);
		const FVector TraceEnd = FootLocation - FVector(0.0f, 0.0f, TraceEndOffset);

		// Simple ground plane intersection (assumes flat ground at ComponentTransform Z level)
		// For complex terrain, pass ground height data as input
		const float GroundZ = ComponentTransform.GetLocation().Z;

		bool bHit = false;
		FVector HitPoint = FVector::ZeroVector;
		FVector HitNormal = FVector::UpVector;

		// Check if trace line intersects with ground plane
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

			// IK target is the hit point plus foot height offset
			FVector TargetLocation = HitPoint + FVector(0.0f, 0.0f, FootHeight);

			// Calculate the foot offset
			FootOffset = TargetLocation.Z - FootLocation.Z;

			// Clamp to max offset
			FootOffset = FMath::Clamp(FootOffset, -MaxIKOffset, MaxIKOffset);

			// Set IK target with clamped offset
			Output.IKTarget = FootLocation + FVector(0.0f, 0.0f, FootOffset);

			// Calculate foot rotation if enabled
			if (bAlignFootToGround && FVector::DotProduct(HitNormal, FVector::UpVector) < 0.99f)
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

	// Process each leg
	float FootOffset_FrontLeft = ProcessLeg(FrontLeftLeg, FrontLeftOutput);
	float FootOffset_FrontRight = ProcessLeg(FrontRightLeg, FrontRightOutput);
	float FootOffset_BackLeft = ProcessLeg(BackLeftLeg, BackLeftOutput);
	float FootOffset_BackRight = ProcessLeg(BackRightLeg, BackRightOutput);

	// Calculate pelvis adjustment
	{
		// Find the minimum (most negative) foot offset
		const float MinOffset = FMath::Min(
			FMath::Min(FootOffset_FrontLeft, FootOffset_FrontRight),
			FMath::Min(FootOffset_BackLeft, FootOffset_BackRight)
		);

		// Only apply pelvis adjustment for negative offsets (lowering the body)
		if (MinOffset < 0.0f)
		{
			PelvisOffset = FVector(0.0f, 0.0f, MinOffset);
		}
		else
		{
			PelvisOffset = FVector::ZeroVector;
		}

		// Calculate pelvis rotation based on front/back and left/right height differences
		const float FrontAvg = (FootOffset_FrontLeft + FootOffset_FrontRight) * 0.5f;
		const float BackAvg = (FootOffset_BackLeft + FootOffset_BackRight) * 0.5f;
		const float LeftAvg = (FootOffset_FrontLeft + FootOffset_BackLeft) * 0.5f;
		const float RightAvg = (FootOffset_FrontRight + FootOffset_BackRight) * 0.5f;

		// Calculate pitch (front-back tilt) and roll (left-right tilt)
		const float PitchAngle = FMath::Atan2(FrontAvg - BackAvg, 100.0f);
		const float RollAngle = FMath::Atan2(RightAvg - LeftAvg, 50.0f);

		// Clamp to reasonable angles (15 degrees max)
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
