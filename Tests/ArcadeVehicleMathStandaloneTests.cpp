#include "ArcadeVehicleMath.h"

#include <cmath>
#include <iostream>
#include <string_view>

using namespace Nextcar::ArcadeVehicleMath;

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

    if (FailureCount != 0)
    {
        std::cerr << FailureCount << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All standalone vehicle math tests passed.\n";
    return 0;
}
