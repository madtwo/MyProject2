#include "GSGravityMathLibrary.h"

FVector UGSGravityMathLibrary::AxisToDirection(const EGSGravityAxis Axis)
{
    switch (Axis)
    {
        case EGSGravityAxis::XPositive:
            return FVector(1.0, 0.0, 0.0);
        case EGSGravityAxis::XNegative:
            return FVector(-1.0, 0.0, 0.0);
        case EGSGravityAxis::YPositive:
            return FVector(0.0, 1.0, 0.0);
        case EGSGravityAxis::YNegative:
            return FVector(0.0, -1.0, 0.0);
        case EGSGravityAxis::ZPositive:
            return FVector(0.0, 0.0, 1.0);
        case EGSGravityAxis::ZNegative:
        default:
            return FVector(0.0, 0.0, -1.0);
    }
}

EGSGravityAxis UGSGravityMathLibrary::OppositeAxis(const EGSGravityAxis Axis)
{
    switch (Axis)
    {
        case EGSGravityAxis::XPositive:
            return EGSGravityAxis::XNegative;
        case EGSGravityAxis::XNegative:
            return EGSGravityAxis::XPositive;
        case EGSGravityAxis::YPositive:
            return EGSGravityAxis::YNegative;
        case EGSGravityAxis::YNegative:
            return EGSGravityAxis::YPositive;
        case EGSGravityAxis::ZPositive:
            return EGSGravityAxis::ZNegative;
        case EGSGravityAxis::ZNegative:
        default:
            return EGSGravityAxis::ZPositive;
    }
}

float UGSGravityMathLibrary::GetGravitySpeed(const FVector& Velocity, const FVector& GravityDirection)
{
    const FVector SafeDirection = GravityDirection.GetSafeNormal();
    return FVector::DotProduct(Velocity, SafeDirection);
}

FVector UGSGravityMathLibrary::GetTangentialVelocity(const FVector& Velocity, const FVector& GravityDirection)
{
    const FVector SafeDirection = GravityDirection.GetSafeNormal();
    return Velocity - SafeDirection * FVector::DotProduct(Velocity, SafeDirection);
}

float UGSGravityMathLibrary::ComputeImpactEnergyJ(const float MassKg, const float RelativeNormalSpeedCms)
{
    const float SafeMassKg = FMath::Max(0.0f, MassKg);
    const float SpeedMps = FMath::Max(0.0f, RelativeNormalSpeedCms) / 100.0f;
    return 0.5f * SafeMassKg * SpeedMps * SpeedMps;
}

EGSImpactTier UGSGravityMathLibrary::SelectImpactTier(
    const float EnergyJ,
    const float LightJ,
    const float HeavyJ,
    const float CriticalJ)
{
    if (EnergyJ >= CriticalJ)
    {
        return EGSImpactTier::Critical;
    }

    if (EnergyJ >= HeavyJ)
    {
        return EGSImpactTier::Heavy;
    }

    if (EnergyJ >= LightJ)
    {
        return EGSImpactTier::Light;
    }

    return EGSImpactTier::None;
}

FGSGravityBasis UGSGravityMathLibrary::BuildCameraBasis(
    const FVector& GravityDirection,
    const FVector& PreferredForward)
{
    FGSGravityBasis Basis;

    FVector Gravity = GravityDirection.GetSafeNormal();
    if (Gravity.IsNearlyZero())
    {
        Gravity = FVector(0.0, 0.0, -1.0);
    }

    Basis.Up = -Gravity;

    FVector Forward = FVector::VectorPlaneProject(PreferredForward, Gravity).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        const FVector Fallback = FMath::Abs(Gravity.Z) < 0.9f
            ? FVector::UpVector
            : FVector::ForwardVector;
        Forward = FVector::VectorPlaneProject(Fallback, Gravity).GetSafeNormal();
    }

    Basis.Forward = Forward;
    Basis.Right = FVector::CrossProduct(Basis.Up, Basis.Forward).GetSafeNormal();

    if (Basis.Right.IsNearlyZero())
    {
        Basis.Right = FVector::RightVector;
    }

    Basis.Forward = FVector::CrossProduct(Basis.Right, Basis.Up).GetSafeNormal();
    return Basis;
}

bool UGSGravityMathLibrary::IsCardinalUnitVector(const FVector& Direction, const float Tolerance)
{
    const FVector SafeDirection = Direction.GetSafeNormal();
    if (SafeDirection.IsNearlyZero())
    {
        return false;
    }

    const float X = FMath::Abs(SafeDirection.X);
    const float Y = FMath::Abs(SafeDirection.Y);
    const float Z = FMath::Abs(SafeDirection.Z);
    const int32 NearOneCount =
        (FMath::IsNearlyEqual(X, 1.0f, Tolerance) ? 1 : 0) +
        (FMath::IsNearlyEqual(Y, 1.0f, Tolerance) ? 1 : 0) +
        (FMath::IsNearlyEqual(Z, 1.0f, Tolerance) ? 1 : 0);

    const int32 NearZeroCount =
        (FMath::IsNearlyZero(X, Tolerance) ? 1 : 0) +
        (FMath::IsNearlyZero(Y, Tolerance) ? 1 : 0) +
        (FMath::IsNearlyZero(Z, Tolerance) ? 1 : 0);

    return NearOneCount == 1 && NearZeroCount == 2;
}
