#pragma once

#include "CoreMinimal.h"

namespace Nextcar::ArcadeVehicleMath
{
struct FSpeedStepParameters
{
    float MaxForwardSpeed = 5000.0f;
    float MaxReverseSpeed = 1500.0f;
    float ForwardAcceleration = 1700.0f;
    float ReverseAcceleration = 1000.0f;
    float BrakeDeceleration = 3600.0f;
    float HandbrakeDeceleration = 5200.0f;
    float CoastDeceleration = 420.0f;
};

inline float StepSpeed(
    const float CurrentSpeed,
    const float ThrottleInput,
    const bool bHandbrakeHeld,
    const float DeltaSeconds,
    const FSpeedStepParameters& Parameters)
{
    if (DeltaSeconds <= 0.0f)
    {
        return CurrentSpeed;
    }

    const float ClampedThrottle = FMath::Clamp(ThrottleInput, -1.0f, 1.0f);
    const bool bThrottleOpposesMotion =
        (ClampedThrottle > 0.0f && CurrentSpeed < -10.0f) ||
        (ClampedThrottle < 0.0f && CurrentSpeed > 10.0f);

    if (bHandbrakeHeld)
    {
        return FMath::FInterpConstantTo(
            CurrentSpeed,
            0.0f,
            DeltaSeconds,
            Parameters.HandbrakeDeceleration);
    }

    if (bThrottleOpposesMotion)
    {
        return FMath::FInterpConstantTo(
            CurrentSpeed,
            0.0f,
            DeltaSeconds,
            Parameters.BrakeDeceleration * FMath::Abs(ClampedThrottle));
    }

    if (!FMath::IsNearlyZero(ClampedThrottle, 0.01f))
    {
        const float TargetSpeed =
            ClampedThrottle > 0.0f ? Parameters.MaxForwardSpeed : -Parameters.MaxReverseSpeed;
        const float Acceleration =
            ClampedThrottle > 0.0f ? Parameters.ForwardAcceleration : Parameters.ReverseAcceleration;

        return FMath::FInterpConstantTo(
            CurrentSpeed,
            TargetSpeed,
            DeltaSeconds,
            Acceleration * FMath::Abs(ClampedThrottle));
    }

    return FMath::FInterpConstantTo(
        CurrentSpeed,
        0.0f,
        DeltaSeconds,
        Parameters.CoastDeceleration);
}

inline float CalculateYawDelta(
    const float SpeedCmPerSecond,
    const float SteeringInput,
    const float DeltaSeconds,
    const float MaxForwardSpeed,
    const float LowSpeedSteeringRate,
    const float HighSpeedSteeringRate)
{
    if (DeltaSeconds <= 0.0f || MaxForwardSpeed <= 0.0f)
    {
        return 0.0f;
    }

    const float ClampedSteering = FMath::Clamp(SteeringInput, -1.0f, 1.0f);
    const float SpeedRatio = FMath::Clamp(
        FMath::Abs(SpeedCmPerSecond) / MaxForwardSpeed,
        0.0f,
        1.0f);
    const float SteeringRate = FMath::Lerp(
        LowSpeedSteeringRate,
        HighSpeedSteeringRate,
        SpeedRatio);
    const float SteeringAuthority = FMath::Clamp(
        FMath::Abs(SpeedCmPerSecond) / 300.0f,
        0.0f,
        1.0f);
    const float TravelDirection = SpeedCmPerSecond < 0.0f ? -1.0f : 1.0f;

    return ClampedSteering *
        SteeringRate *
        SteeringAuthority *
        TravelDirection *
        DeltaSeconds;
}

inline float CentimetersPerSecondToKilometersPerHour(const float SpeedCmPerSecond)
{
    return FMath::Abs(SpeedCmPerSecond) * 0.036f;
}
}
