#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GravityShiftTypes.h"
#include "GravityShiftProfiles.generated.h"

UCLASS(BlueprintType)
class GRAVITYSHIFTCORE_API UGSGravityRuleSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UGSGravityRuleSet();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
    EGSGravityAxis InitialAxis = EGSGravityAxis::ZNegative;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
    TArray<EGSGravityAxis> AllowedAxes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity", meta = (ClampMin = "0.0", Units = "cm/s^2"))
    float GravityMagnitudeCms2 = 980.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity", meta = (ClampMin = "0.0", Units = "s"))
    float ManualChangeCooldown = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity", meta = (ClampMin = "0.0", Units = "s"))
    float LandingAutoReverseCooldown = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VelocityRetentionOnShift = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity", meta = (ClampMin = "0.0", Units = "cm/s"))
    float MaxGravitySpeedCms = 3200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
    bool bCycleAllowedAxes = true;
};

UCLASS(BlueprintType)
class GRAVITYSHIFTCORE_API UGSSurfaceProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
    FGSSurfaceModifier Modifier;
};

UCLASS(BlueprintType)
class GRAVITYSHIFTCORE_API UGSBreakProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = "0.0"))
    float MinEnergyJ = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = "0.0"))
    float InstantBreakEnergyJ = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = "0.0"))
    float DamagePerJoule = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = "0.0"))
    float MaxDamagePerHit = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable")
    bool bInstantBreakAboveThreshold = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable")
    EGSDestructionMode DestructionMode = EGSDestructionMode::HideAndDisable;
};

UCLASS(BlueprintType)
class GRAVITYSHIFTCORE_API UGSBlockProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
    bool bSimulatePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
    bool bGravityAffected = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
    bool bBreakable = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0.001", Units = "kg"))
    float MassOverrideKg = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block", meta = (ClampMin = "0.0"))
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
    bool bUseCCD = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
    TObjectPtr<UGSBreakProfile> BreakProfile = nullptr;
};
