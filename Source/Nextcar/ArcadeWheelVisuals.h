#pragma once

#include "CoreMinimal.h"

namespace Nextcar::ArcadeWheelVisuals
{
inline FRotator MakeWheelRotation(float SteeringAngleDegrees, float WheelRotationDegrees)
{
    // The engine cylinder mesh uses local Z as its symmetry axis. Roll it by 90 degrees
    // so the axle lies across the car, then apply rolling around that axle and steering
    // around the vehicle's vertical axis.
    return FRotator(WheelRotationDegrees, SteeringAngleDegrees, 90.0f);
}
}
