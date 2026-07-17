#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArcadeCarPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class NEXTCAR_API AArcadeCarPawn : public APawn
{
    GENERATED_BODY()

public:
    AArcadeCarPawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category = "Vehicle")
    float GetSpeedKmh() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UBoxComponent> CollisionBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> CabinMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontLeftWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontRightWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearLeftWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearRightWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> ChaseCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Speed", meta = (ClampMin = "100.0"))
    float MaxForwardSpeed = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Speed", meta = (ClampMin = "100.0"))
    float MaxReverseSpeed = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Acceleration", meta = (ClampMin = "0.0"))
    float ForwardAcceleration = 1700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Acceleration", meta = (ClampMin = "0.0"))
    float ReverseAcceleration = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Acceleration", meta = (ClampMin = "0.0"))
    float BrakeDeceleration = 3600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Acceleration", meta = (ClampMin = "0.0"))
    float HandbrakeDeceleration = 5200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Acceleration", meta = (ClampMin = "0.0"))
    float CoastDeceleration = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Steering", meta = (ClampMin = "0.0"))
    float LowSpeedSteeringRate = 105.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving|Steering", meta = (ClampMin = "0.0"))
    float HighSpeedSteeringRate = 34.0f;

private:
    void SetThrottleInput(float Value);
    void SetSteeringInput(float Value);
    void PressHandbrake();
    void ReleaseHandbrake();
    void ResetCar();
    void ConfigureWheel(UStaticMeshComponent* Wheel, const FVector& RelativeLocation);
    void UpdateWheelVisuals(float DeltaSeconds, float SteeringAngleDegrees);

    float ThrottleInput = 0.0f;
    float SteeringInput = 0.0f;
    float SpeedCmPerSecond = 0.0f;
    float WheelRotationDegrees = 0.0f;
    bool bHandbrakeHeld = false;

    FVector ResetLocation = FVector::ZeroVector;
    FRotator ResetRotation = FRotator::ZeroRotator;
};
