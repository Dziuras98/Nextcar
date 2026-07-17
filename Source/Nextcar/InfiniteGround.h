#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfiniteGround.generated.h"

class APawn;
class UStaticMeshComponent;

UCLASS()
class NEXTCAR_API AInfiniteGround : public AActor
{
    GENERATED_BODY()

public:
    AInfiniteGround();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ground")
    TObjectPtr<UStaticMeshComponent> GroundMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "10000.0"))
    float GroundSize = 40000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground", meta = (ClampMin = "1000.0"))
    float RecenterDistance = 10000.0f;

private:
    TWeakObjectPtr<APawn> TrackedPawn;
};
