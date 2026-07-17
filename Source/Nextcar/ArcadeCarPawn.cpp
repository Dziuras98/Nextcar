#include "ArcadeCarPawn.h"

#include "ArcadeVehicleMath.h"
#include "ArcadeVehicleSimulation.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AArcadeCarPawn::AArcadeCarPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBody = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBody"));
    SetRootComponent(CollisionBody);
    CollisionBody->SetBoxExtent(FVector(110.0f, 58.0f, 24.0f));
    CollisionBody->SetCollisionProfileName(TEXT("Pawn"));
    CollisionBody->SetSimulatePhysics(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(CollisionBody);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeScale3D(FVector(2.20f, 1.10f, 0.46f));
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));

    CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
    CabinMesh->SetupAttachment(CollisionBody);
    CabinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CabinMesh->SetRelativeScale3D(FVector(1.05f, 0.88f, 0.38f));
    CabinMesh->SetRelativeLocation(FVector(-18.0f, 0.0f, 43.0f));

    FrontLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftWheel"));
    FrontRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightWheel"));
    RearLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearLeftWheel"));
    RearRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearRightWheel"));

    if (CubeMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMesh.Object);
        CabinMesh->SetStaticMesh(CubeMesh.Object);
    }

    if (CylinderMesh.Succeeded())
    {
        FrontLeftWheel->SetStaticMesh(CylinderMesh.Object);
        FrontRightWheel->SetStaticMesh(CylinderMesh.Object);
        RearLeftWheel->SetStaticMesh(CylinderMesh.Object);
        RearRightWheel->SetStaticMesh(CylinderMesh.Object);
    }

    ConfigureWheel(FrontLeftWheel, FVector(73.0f, -61.0f, -14.0f));
    ConfigureWheel(FrontRightWheel, FVector(73.0f, 61.0f, -14.0f));
    ConfigureWheel(RearLeftWheel, FVector(-73.0f, -61.0f, -14.0f));
    ConfigureWheel(RearRightWheel, FVector(-73.0f, 61.0f, -14.0f));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(CollisionBody);
    CameraBoom->TargetArmLength = 650.0f;
    CameraBoom->SetRelativeLocation(FVector(-20.0f, 0.0f, 95.0f));
    CameraBoom->SetRelativeRotation(FRotator(-12.0f, 0.0f, 0.0f));
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 8.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 10.0f;
    CameraBoom->bDoCollisionTest = true;

    ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
    ChaseCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ChaseCamera->FieldOfView = 78.0f;

    AutoPossessPlayer = EAutoReceiveInput::Player0;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}

void AArcadeCarPawn::BeginPlay()
{
    Super::BeginPlay();

    FVector StartLocation = GetActorLocation();
    StartLocation.Z = 45.0f;
    SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);

    ResetLocation = GetActorLocation();
    ResetRotation = GetActorRotation();
}

void AArcadeCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    Nextcar::ArcadeVehicleSimulation::FVehicleTuning Tuning;
    Tuning.Speed.MaxForwardSpeed = MaxForwardSpeed;
    Tuning.Speed.MaxReverseSpeed = MaxReverseSpeed;
    Tuning.Speed.ForwardAcceleration = ForwardAcceleration;
    Tuning.Speed.ReverseAcceleration = ReverseAcceleration;
    Tuning.Speed.BrakeDeceleration = BrakeDeceleration;
    Tuning.Speed.HandbrakeDeceleration = HandbrakeDeceleration;
    Tuning.Speed.CoastDeceleration = CoastDeceleration;
    Tuning.LowSpeedSteeringRate = LowSpeedSteeringRate;
    Tuning.HighSpeedSteeringRate = HighSpeedSteeringRate;
    Tuning.MaxSteeringAngleDegrees = MaxSteeringAngleDegrees;
    Tuning.WheelRadiusCm = WheelRadiusCm;
    Tuning.SimulationSubstepSeconds = SimulationSubstepSeconds;

    const Nextcar::ArcadeVehicleSimulation::FVehicleState CurrentState{
        SpeedCmPerSecond,
        WheelRotationDegrees};
    const Nextcar::ArcadeVehicleSimulation::FVehicleInput Input{
        ThrottleInput,
        SteeringInput,
        bHandbrakeHeld};
    const Nextcar::ArcadeVehicleSimulation::FVehicleStepResult StepResult =
        Nextcar::ArcadeVehicleSimulation::StepVehicle(
            CurrentState,
            Input,
            DeltaSeconds,
            Tuning);

    SpeedCmPerSecond = StepResult.State.SpeedCmPerSecond;
    WheelRotationDegrees = StepResult.State.WheelRotationDegrees;

    AddActorWorldRotation(FRotator(0.0f, StepResult.YawDeltaDegrees, 0.0f));

    const FVector Translation = GetActorForwardVector() * StepResult.TravelDistanceCm;
    FHitResult Hit;
    AddActorWorldOffset(Translation, true, &Hit);

    if (Hit.bBlockingHit)
    {
        SpeedCmPerSecond = 0.0f;
    }

    UpdateWheelVisuals(StepResult.SteeringAngleDegrees);
}

void AArcadeCarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("Throttle"), this, &AArcadeCarPawn::SetThrottleInput);
    PlayerInputComponent->BindAxis(TEXT("Steer"), this, &AArcadeCarPawn::SetSteeringInput);
    PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &AArcadeCarPawn::PressHandbrake);
    PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Released, this, &AArcadeCarPawn::ReleaseHandbrake);
    PlayerInputComponent->BindAction(TEXT("ResetCar"), IE_Pressed, this, &AArcadeCarPawn::ResetCar);
}

float AArcadeCarPawn::GetSpeedKmh() const
{
    return Nextcar::ArcadeVehicleMath::CentimetersPerSecondToKilometersPerHour(SpeedCmPerSecond);
}

void AArcadeCarPawn::SetThrottleInput(float Value)
{
    ThrottleInput = Value;
}

void AArcadeCarPawn::SetSteeringInput(float Value)
{
    SteeringInput = Value;
}

void AArcadeCarPawn::PressHandbrake()
{
    bHandbrakeHeld = true;
}

void AArcadeCarPawn::ReleaseHandbrake()
{
    bHandbrakeHeld = false;
}

void AArcadeCarPawn::ResetCar()
{
    SpeedCmPerSecond = 0.0f;
    ThrottleInput = 0.0f;
    SteeringInput = 0.0f;
    WheelRotationDegrees = 0.0f;
    bHandbrakeHeld = false;
    SetActorLocationAndRotation(
        ResetLocation,
        ResetRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    UpdateWheelVisuals(0.0f);
}

void AArcadeCarPawn::ConfigureWheel(UStaticMeshComponent* Wheel, const FVector& RelativeLocation)
{
    check(Wheel);
    Wheel->SetupAttachment(CollisionBody);
    Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Wheel->SetRelativeLocation(RelativeLocation);
    Wheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    Wheel->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.18f));
}

void AArcadeCarPawn::UpdateWheelVisuals(float SteeringAngleDegrees)
{
    const FRotator FrontWheelRotation(90.0f, SteeringAngleDegrees, WheelRotationDegrees);
    const FRotator RearWheelRotation(90.0f, 0.0f, WheelRotationDegrees);

    FrontLeftWheel->SetRelativeRotation(FrontWheelRotation);
    FrontRightWheel->SetRelativeRotation(FrontWheelRotation);
    RearLeftWheel->SetRelativeRotation(RearWheelRotation);
    RearRightWheel->SetRelativeRotation(RearWheelRotation);
}
