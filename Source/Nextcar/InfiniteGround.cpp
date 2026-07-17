#include "InfiniteGround.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AInfiniteGround::AInfiniteGround()
{
    PrimaryActorTick.bCanEverTick = true;

    GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
    SetRootComponent(GroundMesh);
    GroundMesh->SetMobility(EComponentMobility::Movable);
    GroundMesh->SetCollisionProfileName(TEXT("BlockAll"));
    GroundMesh->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GridMaterial(
        TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));

    if (CubeMesh.Succeeded())
    {
        GroundMesh->SetStaticMesh(CubeMesh.Object);
    }

    if (GridMaterial.Succeeded())
    {
        GroundMesh->SetMaterial(0, GridMaterial.Object);
    }

    GroundMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
}

void AInfiniteGround::BeginPlay()
{
    Super::BeginPlay();

    GroundMesh->SetWorldScale3D(FVector(GroundSize / 100.0f, GroundSize / 100.0f, 0.2f));
    TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
}

void AInfiniteGround::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!TrackedPawn.IsValid())
    {
        TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    }

    if (!TrackedPawn.IsValid())
    {
        return;
    }

    const FVector PawnLocation = TrackedPawn->GetActorLocation();
    const FVector GroundLocation = GetActorLocation();
    const FVector2D Offset(PawnLocation.X - GroundLocation.X, PawnLocation.Y - GroundLocation.Y);

    if (Offset.SizeSquared() < FMath::Square(RecenterDistance))
    {
        return;
    }

    const FVector NewLocation(
        FMath::GridSnap(PawnLocation.X, RecenterDistance),
        FMath::GridSnap(PawnLocation.Y, RecenterDistance),
        0.0f);

    SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
}
