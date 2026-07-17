#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NextcarGameMode.generated.h"

UCLASS()
class NEXTCAR_API ANextcarGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ANextcarGameMode();

    virtual void StartPlay() override;
};
