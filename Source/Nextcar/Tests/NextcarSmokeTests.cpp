#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "../ArcadeCarPawn.h"
#include "../ArcadeWheelVisuals.h"
#include "../InfiniteGround.h"
#include "../NextcarGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNextcarClassConstructionTest,
    "Nextcar.Smoke.ClassConstruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNextcarClassConstructionTest::RunTest(const FString& Parameters)
{
    const AArcadeCarPawn* CarDefaults = GetDefault<AArcadeCarPawn>();
    TestNotNull(TEXT("Arcade car class default object exists"), CarDefaults);

    if (CarDefaults != nullptr)
    {
        TestNotNull(
            TEXT("Arcade car contains a collision body"),
            CarDefaults->FindComponentByClass<UBoxComponent>());
        TestNotNull(
            TEXT("Arcade car contains a chase camera"),
            CarDefaults->FindComponentByClass<UCameraComponent>());
        TestNotNull(
            TEXT("Arcade car contains a spring arm"),
            CarDefaults->FindComponentByClass<USpringArmComponent>());
    }

    const ANextcarGameMode* GameModeDefaults = GetDefault<ANextcarGameMode>();
    TestNotNull(TEXT("Game mode class default object exists"), GameModeDefaults);
    if (GameModeDefaults != nullptr)
    {
        TestEqual(
            TEXT("Game mode uses the arcade car as its default pawn"),
            GameModeDefaults->DefaultPawnClass.Get(),
            AArcadeCarPawn::StaticClass());
    }

    TestNotNull(TEXT("Infinite ground class default object exists"), GetDefault<AInfiniteGround>());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNextcarWheelVisualAxesTest,
    "Nextcar.Smoke.WheelVisualAxes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNextcarWheelVisualAxesTest::RunTest(const FString& Parameters)
{
    using Nextcar::ArcadeWheelVisuals::MakeWheelRotation;

    const FVector BaseAxle = MakeWheelRotation(0.0f, 0.0f)
                                  .RotateVector(FVector::UpVector)
                                  .GetSafeNormal();
    TestTrue(
        TEXT("Wheel cylinder axle is aligned with the car lateral axis"),
        FMath::IsNearlyEqual(
            FMath::Abs(FVector::DotProduct(BaseAxle, FVector::RightVector)),
            1.0f,
            KINDA_SMALL_NUMBER));

    const FVector SpunAxle = MakeWheelRotation(0.0f, 90.0f)
                                  .RotateVector(FVector::UpVector)
                                  .GetSafeNormal();
    TestTrue(
        TEXT("Rolling rotation preserves the wheel axle"),
        BaseAxle.Equals(SpunAxle, KINDA_SMALL_NUMBER));

    const FVector BaseRadius = MakeWheelRotation(0.0f, 0.0f)
                                    .RotateVector(FVector::ForwardVector)
                                    .GetSafeNormal();
    const FVector SpunRadius = MakeWheelRotation(0.0f, 90.0f)
                                    .RotateVector(FVector::ForwardVector)
                                    .GetSafeNormal();
    TestTrue(
        TEXT("Rolling rotation moves a radial direction around the axle"),
        FMath::IsNearlyZero(
            FVector::DotProduct(BaseRadius, SpunRadius),
            KINDA_SMALL_NUMBER));

    constexpr float SteeringAngleDegrees = 25.0f;
    const FVector SteeredAxle = MakeWheelRotation(SteeringAngleDegrees, 90.0f)
                                     .RotateVector(FVector::UpVector)
                                     .GetSafeNormal();
    const FVector ExpectedSteeredAxle = FRotator(0.0f, SteeringAngleDegrees, 0.0f)
                                             .RotateVector(BaseAxle)
                                             .GetSafeNormal();
    TestTrue(
        TEXT("Steering rotates the wheel axle around the vehicle vertical axis"),
        SteeredAxle.Equals(ExpectedSteeredAxle, KINDA_SMALL_NUMBER));

    return true;
}

#endif
