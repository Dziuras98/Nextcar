#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NextcarHUD.generated.h"

UCLASS()
class NEXTCAR_API ANextcarHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
