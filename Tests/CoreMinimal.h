#pragma once

#include <algorithm>
#include <cmath>

struct FMath
{
    template <typename T>
    static constexpr T Clamp(const T Value, const T Minimum, const T Maximum)
    {
        return std::clamp(Value, Minimum, Maximum);
    }

    template <typename T>
    static constexpr T Abs(const T Value)
    {
        return Value < static_cast<T>(0) ? -Value : Value;
    }

    template <typename T, typename AlphaType>
    static constexpr T Lerp(const T A, const T B, const AlphaType Alpha)
    {
        return A + (B - A) * Alpha;
    }

    static bool IsNearlyZero(const float Value, const float Tolerance)
    {
        return std::fabs(Value) <= Tolerance;
    }

    static float FInterpConstantTo(
        const float Current,
        const float Target,
        const float DeltaTime,
        const float InterpSpeed)
    {
        if (DeltaTime <= 0.0f || InterpSpeed <= 0.0f)
        {
            return Current;
        }

        const float Distance = Target - Current;
        const float MaximumStep = InterpSpeed * DeltaTime;
        if (std::fabs(Distance) <= MaximumStep)
        {
            return Target;
        }

        return Current + std::copysign(MaximumStep, Distance);
    }
};
