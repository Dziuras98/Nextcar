#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ArcadeCarPawn.h"
#include "InfiniteGround.h"
#include "NextcarGameMode.h"
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
            GameModeDefaults->DefaultPawnClass,
            AArcadeCarPawn::StaticClass());
    }

    TestNotNull(TEXT("Infinite ground class default object exists"), GetDefault<AInfiniteGround>());
    return true;
}

#endif
