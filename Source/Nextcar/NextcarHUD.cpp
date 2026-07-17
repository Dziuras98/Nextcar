#include "NextcarHUD.h"

#include "ArcadeCarPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void ANextcarHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !PlayerOwner)
    {
        return;
    }

    const AArcadeCarPawn* Car = Cast<AArcadeCarPawn>(PlayerOwner->GetPawn());
    const float SpeedKmh = Car ? Car->GetSpeedKmh() : 0.0f;

    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
    if (!Font)
    {
        return;
    }

    Canvas->DrawText(
        Font,
        FString::Printf(TEXT("%03.0f km/h"), SpeedKmh),
        40.0f,
        40.0f,
        1.6f,
        1.6f);

    Canvas->DrawText(
        Font,
        TEXT("W/S: accelerate/reverse   A/D: steer   Space: handbrake   R: reset"),
        40.0f,
        82.0f,
        1.0f,
        1.0f);
}
