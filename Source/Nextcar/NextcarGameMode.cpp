#include "NextcarGameMode.h"

#include "ArcadeCarPawn.h"
#include "InfiniteGround.h"
#include "NextcarHUD.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

ANextcarGameMode::ANextcarGameMode()
{
    DefaultPawnClass = AArcadeCarPawn::StaticClass();
    HUDClass = ANextcarHUD::StaticClass();
}

void ANextcarGameMode::StartPlay()
{
    Super::StartPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bool bHasGround = false;
    for (TActorIterator<AInfiniteGround> It(World); It; ++It)
    {
        bHasGround = true;
        break;
    }

    if (!bHasGround)
    {
        World->SpawnActor<AInfiniteGround>(
            AInfiniteGround::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!PlayerController)
    {
        return;
    }

    AArcadeCarPawn* Car = Cast<AArcadeCarPawn>(PlayerController->GetPawn());
    if (!Car)
    {
        Car = World->SpawnActor<AArcadeCarPawn>(
            AArcadeCarPawn::StaticClass(),
            FVector(0.0f, 0.0f, 45.0f),
            FRotator::ZeroRotator);

        if (Car)
        {
            PlayerController->Possess(Car);
        }
    }
    else
    {
        FVector CarLocation = Car->GetActorLocation();
        CarLocation.Z = FMath::Max(CarLocation.Z, 45.0f);
        Car->SetActorLocation(CarLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
}
