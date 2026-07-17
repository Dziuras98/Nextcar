#pragma once

#include "ArcadeVehicleMath.h"
#include "CoreMinimal.h"

namespace Nextcar::ArcadeVehicleSimulation
{
struct FVehicleInput
{
    float Throttle = 0.0f;
    float Steering = 0.0f;
    bool bHandbrakeHeld = false;
};

struct FVehicleTuning
{
    ArcadeVehicleMath::FSpeedStepParameters Speed;
    float LowSpeedSteeringRate = 105.0f;
    float HighSpeedSteeringRate = 34.0f;
    float MaxSteeringAngleDegrees = 28.0f;
    float WheelRadiusCm = 34.0f;
    float SimulationSubstepSeconds = 1.0f / 120.0f;
};

struct FVehicleState
{
    float SpeedCmPerSecond = 0.0f;
    float WheelRotationDegrees = 0.0f;
};

struct FVehicleStepResult
{
    FVehicleState State;
    float TravelDistanceCm = 0.0f;
    float YawDeltaDegrees = 0.0f;
    float SteeringAngleDegrees = 0.0f;
};

inline float NormalizeRotationDegrees(const float Degrees)
{
    float Normalized = FMath::Fmod(Degrees, 360.0f);
    if (Normalized < 0.0f)
    {
        Normalized += 360.0f;
    }
    return Normalized;
}

inline float CalculateWheelRotationDeltaDegrees(
    const float TravelDistanceCm,
    const float WheelRadiusCm)
{
    if (WheelRadiusCm <= 0.0f)
    {
        return 0.0f;
    }

    constexpr float Pi = 3.14159265358979323846f;
    const float CircumferenceCm = 2.0f * Pi * WheelRadiusCm;
    return (TravelDistanceCm / CircumferenceCm) * 360.0f;
}

inline FVehicleStepResult StepVehicle(
    const FVehicleState& CurrentState,
    const FVehicleInput& Input,
    const float DeltaSeconds,
    const FVehicleTuning& Tuning)
{
    FVehicleStepResult Result;
    Result.State = CurrentState;
    Result.SteeringAngleDegrees =
        FMath::Clamp(Input.Steering, -1.0f, 1.0f) * Tuning.MaxSteeringAngleDegrees;

    if (DeltaSeconds <= 0.0f)
    {
        return Result;
    }

    const float MaximumSubstep =
        Tuning.SimulationSubstepSeconds > 0.0f ? Tuning.SimulationSubstepSeconds : DeltaSeconds;
    float RemainingSeconds = DeltaSeconds;

    while (RemainingSeconds > 0.0f)
    {
        const float StepSeconds = FMath::Min(RemainingSeconds, MaximumSubstep);
        const float PreviousSpeed = Result.State.SpeedCmPerSecond;
        const float NewSpeed = ArcadeVehicleMath::StepSpeed(
            PreviousSpeed,
            Input.Throttle,
            Input.bHandbrakeHeld,
            StepSeconds,
            Tuning.Speed);
        const float AverageSpeed = (PreviousSpeed + NewSpeed) * 0.5f;

        Result.TravelDistanceCm += AverageSpeed * StepSeconds;
        Result.YawDeltaDegrees += ArcadeVehicleMath::CalculateYawDelta(
            AverageSpeed,
            Input.Steering,
            StepSeconds,
            Tuning.Speed.MaxForwardSpeed,
            Tuning.LowSpeedSteeringRate,
            Tuning.HighSpeedSteeringRate);
        Result.State.SpeedCmPerSecond = NewSpeed;
        RemainingSeconds -= StepSeconds;
    }

    Result.State.WheelRotationDegrees = NormalizeRotationDegrees(
        CurrentState.WheelRotationDegrees +
        CalculateWheelRotationDeltaDegrees(Result.TravelDistanceCm, Tuning.WheelRadiusCm));

    return Result;
}
}
