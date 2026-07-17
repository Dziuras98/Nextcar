#if WITH_DEV_AUTOMATION_TESTS

#include "../ArcadeVehicleMath.h"
#include "Misc/AutomationTest.h"

using namespace Nextcar::ArcadeVehicleMath;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArcadeVehicleAccelerationTest,
    "Nextcar.Vehicle.ArcadeMath.Acceleration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcadeVehicleAccelerationTest::RunTest(const FString& Parameters)
{
    const FSpeedStepParameters Settings;

    TestEqual(
        TEXT("Full throttle accelerates by the configured amount in one second"),
        StepSpeed(0.0f, 1.0f, false, 1.0f, Settings),
        Settings.ForwardAcceleration);

    TestEqual(
        TEXT("Forward speed is capped"),
        StepSpeed(Settings.MaxForwardSpeed - 100.0f, 1.0f, false, 1.0f, Settings),
        Settings.MaxForwardSpeed);

    TestEqual(
        TEXT("Reverse throttle accelerates backwards"),
        StepSpeed(0.0f, -1.0f, false, 1.0f, Settings),
        -Settings.ReverseAcceleration);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArcadeVehicleBrakingTest,
    "Nextcar.Vehicle.ArcadeMath.Braking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcadeVehicleBrakingTest::RunTest(const FString& Parameters)
{
    const FSpeedStepParameters Settings;

    TestEqual(
        TEXT("Opposite throttle brakes before reversing"),
        StepSpeed(2000.0f, -1.0f, false, 0.25f, Settings),
        1100.0f);

    TestEqual(
        TEXT("Handbrake cannot cross through zero"),
        StepSpeed(500.0f, 1.0f, true, 1.0f, Settings),
        0.0f);

    TestEqual(
        TEXT("Coasting reduces speed"),
        StepSpeed(1000.0f, 0.0f, false, 1.0f, Settings),
        580.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArcadeVehicleSteeringTest,
    "Nextcar.Vehicle.ArcadeMath.Steering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcadeVehicleSteeringTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("Stationary vehicle has no steering authority"),
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

    TestTrue(TEXT("Forward steering rotates in the requested direction"), ForwardYaw > 0.0f);
    TestEqual(TEXT("Reverse steering direction is inverted"), ReverseYaw, -ForwardYaw);

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

    TestTrue(TEXT("Steering sensitivity decreases at high speed"), HighSpeedYaw < LowSpeedYaw);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FArcadeVehicleUnitsTest,
    "Nextcar.Vehicle.ArcadeMath.Units",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcadeVehicleUnitsTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("1000 cm/s converts to 36 km/h"),
        CentimetersPerSecondToKilometersPerHour(1000.0f),
        36.0f);
    TestEqual(
        TEXT("Displayed speed is absolute when reversing"),
        CentimetersPerSecondToKilometersPerHour(-1000.0f),
        36.0f);

    return true;
}

#endif
