#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GravityShiftTypes.h"
#include "GSGravityMathLibrary.generated.h"

UCLASS()
class GRAVITYSHIFTCORE_API UGSGravityMathLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static FVector AxisToDirection(EGSGravityAxis Axis);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static EGSGravityAxis OppositeAxis(EGSGravityAxis Axis);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static float GetGravitySpeed(const FVector& Velocity, const FVector& GravityDirection);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static FVector GetTangentialVelocity(const FVector& Velocity, const FVector& GravityDirection);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math", meta = ())
    static float ComputeImpactEnergyJ(float MassKg, float RelativeNormalSpeedCms);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static EGSImpactTier SelectImpactTier(float EnergyJ, float LightJ = 150.0f, float HeavyJ = 600.0f, float CriticalJ = 1500.0f);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static FGSGravityBasis BuildCameraBasis(const FVector& GravityDirection, const FVector& PreferredForward);

    UFUNCTION(BlueprintPure, Category = "Gravity Shift|Math")
    static bool IsCardinalUnitVector(const FVector& Direction, float Tolerance = 0.001f);
};
