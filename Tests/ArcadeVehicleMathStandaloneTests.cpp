#include "ArcadeVehicleMath.h"
#include "ArcadeVehicleSimulation.h"

#include <cmath>
#include <iostream>
#include <string_view>

using namespace Nextcar::ArcadeVehicleMath;
using namespace Nextcar::ArcadeVehicleSimulation;

namespace
{
int FailureCount = 0;

void ExpectNear(
    const std::string_view Name,
    const float Actual,
    const float Expected,
    const float Tolerance = 0.001f)
{
    if (std::fabs(Actual - Expected) <= Tolerance)
    {
        std::cout << "[PASS] " << Name << '\n';
        return;
    }

    std::cerr << "[FAIL] " << Name << ": expected " << Expected << ", got " << Actual << '\n';
    ++FailureCount;
}

void ExpectTrue(const std::string_view Name, const bool Condition)
{
    if (Condition)
    {
        std::cout << "[PASS] " << Name << '\n';
        return;
    }

    std::cerr << "[FAIL] " << Name << '\n';
    ++FailureCount;
}
}

int main()
{
    const FSpeedStepParameters Settings;

    ExpectNear(
        "Full throttle accelerates by the configured amount in one second",
        StepSpeed(0.0f, 1.0f, false, 1.0f, Settings),
        Settings.ForwardAcceleration);
    ExpectNear(
        "Forward speed is capped",
        StepSpeed(Settings.MaxForwardSpeed - 100.0f, 1.0f, false, 1.0f, Settings),
        Settings.MaxForwardSpeed);
    ExpectNear(
        "Reverse throttle accelerates backwards",
        StepSpeed(0.0f, -1.0f, false, 1.0f, Settings),
        -Settings.ReverseAcceleration);
    ExpectNear(
        "Opposite throttle brakes before reversing",
        StepSpeed(2000.0f, -1.0f, false, 0.25f, Settings),
        1100.0f);
    ExpectNear(
        "Handbrake cannot cross through zero",
        StepSpeed(500.0f, 1.0f, true, 1.0f, Settings),
        0.0f);
    ExpectNear(
        "Coasting reduces speed",
        StepSpeed(1000.0f, 0.0f, false, 1.0f, Settings),
        580.0f);
    ExpectNear(
        "Zero delta time preserves speed",
        StepSpeed(750.0f, 1.0f, false, 0.0f, Settings),
        750.0f);

    ExpectNear(
        "Stationary vehicle has no steering authority",
        CalculateYawDelta(0.0f, 1.0f, 1.0f, 5000.0f, 105.0f, 34.0f),
        0.0f);

    const float ForwardYaw = CalculateYawDelta(
        1000.0f,
        1.0f,
        1.0f,
        5000.0f,
        105.0f,
        34.0f);
    const float ReverseYaw = CalculateYawDelta(
        -1000.0f,
        1.0f,
        1.0f,
        5000.0f,
        105.0f,
        34.0f);
    const float LowSpeedYaw = CalculateYawDelta(
        500.0f,
        1.0f,
        1.0f,
        5000.0f,
        105.0f,
        34.0f);
    const float HighSpeedYaw = CalculateYawDelta(
        5000.0f,
        1.0f,
        1.0f,
        5000.0f,
        105.0f,
        34.0f);

    ExpectTrue("Forward steering rotates in the requested direction", ForwardYaw > 0.0f);
    ExpectNear("Reverse steering direction is inverted", ReverseYaw, -ForwardYaw);
    ExpectTrue("Steering sensitivity decreases at high speed", HighSpeedYaw < LowSpeedYaw);
    ExpectNear(
        "Steering input is clamped",
        CalculateYawDelta(1000.0f, 2.0f, 1.0f, 5000.0f, 105.0f, 34.0f),
        ForwardYaw);

    ExpectNear(
        "1000 cm/s converts to 36 km/h",
        CentimetersPerSecondToKilometersPerHour(1000.0f),
        36.0f);
    ExpectNear(
        "Displayed speed is absolute when reversing",
        CentimetersPerSecondToKilometersPerHour(-1000.0f),
        36.0f);

    const FVehicleTuning Tuning;
    const FVehicleInput FullThrottleAndSteer{1.0f, 0.6f, false};
    const FVehicleStepResult OneSecond = StepVehicle(
        FVehicleState{},
        FullThrottleAndSteer,
        1.0f,
        Tuning);

    ExpectNear(
        "Integrated state reaches the expected one-second speed",
        OneSecond.State.SpeedCmPerSecond,
        Tuning.Speed.ForwardAcceleration,
        0.01f);
    ExpectNear(
        "Trapezoidal integration uses average speed for distance",
        OneSecond.TravelDistanceCm,
        Tuning.Speed.ForwardAcceleration * 0.5f,
        0.1f);
    ExpectTrue("Integrated steering produces yaw", OneSecond.YawDeltaDegrees > 0.0f);
    ExpectTrue(
        "Wheel rotation is normalized",
        OneSecond.State.WheelRotationDegrees >= 0.0f &&
            OneSecond.State.WheelRotationDegrees < 360.0f);
    ExpectNear(
        "Visual steering angle is clamped and scaled",
        OneSecond.SteeringAngleDegrees,
        0.6f * Tuning.MaxSteeringAngleDegrees);

    FVehicleState PartitionedState;
    float PartitionedDistance = 0.0f;
    float PartitionedYaw = 0.0f;
    for (int Frame = 0; Frame < 60; ++Frame)
    {
        const FVehicleStepResult FrameResult = StepVehicle(
            PartitionedState,
            FullThrottleAndSteer,
            1.0f / 60.0f,
            Tuning);
        PartitionedState = FrameResult.State;
        PartitionedDistance += FrameResult.TravelDistanceCm;
        PartitionedYaw += FrameResult.YawDeltaDegrees;
    }

    ExpectNear(
        "Speed is independent of outer frame partitioning",
        PartitionedState.SpeedCmPerSecond,
        OneSecond.State.SpeedCmPerSecond,
        0.05f);
    ExpectNear(
        "Distance is independent of outer frame partitioning",
        PartitionedDistance,
        OneSecond.TravelDistanceCm,
        0.1f);
    ExpectNear(
        "Yaw is stable across outer frame partitioning",
        PartitionedYaw,
        OneSecond.YawDeltaDegrees,
        0.05f);
    ExpectNear(
        "Wheel rotation is stable across outer frame partitioning",
        PartitionedState.WheelRotationDegrees,
        OneSecond.State.WheelRotationDegrees,
        0.05f);

    const FVehicleStepResult NoTime = StepVehicle(
        OneSecond.State,
        FVehicleInput{-1.0f, -1.0f, true},
        0.0f,
        Tuning);
    ExpectNear(
        "Zero-time simulation preserves speed",
        NoTime.State.SpeedCmPerSecond,
        OneSecond.State.SpeedCmPerSecond);
    ExpectNear(
        "Zero-time simulation preserves wheel rotation",
        NoTime.State.WheelRotationDegrees,
        OneSecond.State.WheelRotationDegrees);
    ExpectNear("Zero-time simulation does not travel", NoTime.TravelDistanceCm, 0.0f);
    ExpectNear("Zero-time simulation does not rotate", NoTime.YawDeltaDegrees, 0.0f);

    FVehicleTuning InvalidWheelTuning = Tuning;
    InvalidWheelTuning.WheelRadiusCm = 0.0f;
    const FVehicleStepResult InvalidWheel = StepVehicle(
        FVehicleState{1000.0f, 42.0f},
        FVehicleInput{},
        0.25f,
        InvalidWheelTuning);
    ExpectNear(
        "Invalid wheel radius preserves visual rotation",
        InvalidWheel.State.WheelRotationDegrees,
        42.0f);

    FVehicleState LongRunState;
    for (int Step = 0; Step < 20000; ++Step)
    {
        const float Phase = static_cast<float>(Step % 600) / 600.0f;
        const FVehicleInput Input{
            Phase < 0.45f ? 1.0f : (Phase < 0.75f ? 0.0f : -0.8f),
            std::sin(static_cast<float>(Step) * 0.013f),
            Step % 997 == 0};
        LongRunState = StepVehicle(LongRunState, Input, 1.0f / 120.0f, Tuning).State;
    }

    ExpectTrue(
        "Long simulation keeps speed finite",
        std::isfinite(LongRunState.SpeedCmPerSecond));
    ExpectTrue(
        "Long simulation keeps wheel rotation finite",
        std::isfinite(LongRunState.WheelRotationDegrees));
    ExpectTrue(
        "Long simulation respects forward speed cap",
        LongRunState.SpeedCmPerSecond <= Tuning.Speed.MaxForwardSpeed + 0.01f);
    ExpectTrue(
        "Long simulation respects reverse speed cap",
        LongRunState.SpeedCmPerSecond >= -Tuning.Speed.MaxReverseSpeed - 0.01f);
    ExpectTrue(
        "Long simulation keeps wheel rotation normalized",
        LongRunState.WheelRotationDegrees >= 0.0f &&
            LongRunState.WheelRotationDegrees < 360.0f);

    if (FailureCount != 0)
    {
        std::cerr << FailureCount << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All standalone vehicle math and simulation tests passed.\n";
    return 0;
}
