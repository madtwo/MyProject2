#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityShiftTypes.h"
#include "GSGravityBodyComponent.generated.h"

class AGSGravityManager;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FGSOnImpactGenerated,
    FGSImpactPayload,
    Payload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FGSOnEffectiveSurfaceChanged,
    FGSSurfaceModifier,
    Modifier);

UCLASS(ClassGroup = (GravityShift), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GRAVITYSHIFTCORE_API UGSGravityBodyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGSGravityBodyComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Target")
    FName TargetComponentTag = TEXT("GravityBody");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Target")
    bool bAllowPrimitiveFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Target")
    bool bAutoEnablePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Runtime")
    bool bRuntimeEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Runtime", meta = (ClampMin = "0.0"))
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Runtime", meta = (ClampMin = "0.0", ClampMax = "1.5"))
    float VelocityRetentionOnShift = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Runtime", meta = (ClampMin = "0.0", Units = "cm/s"))
    float BaseTerminalSpeedCms = 3200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Runtime")
    bool bUseCCD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact")
    bool bCanDealImpact = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0"))
    float BreakPowerMinEnergyJ = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0"))
    float LightEnergyJ = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0"))
    float HeavyEnergyJ = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0"))
    float CriticalEnergyJ = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0"))
    float ImpactEnergyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Impact", meta = (ClampMin = "0.0", Units = "s"))
    float HitDebounceSeconds = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Manager")
    bool bAutoFindManager = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Manager", meta = (ClampMin = "0.05", Units = "s"))
    float ManagerRetrySeconds = 0.50f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    TObjectPtr<UPrimitiveComponent> TargetPrimitive = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    TObjectPtr<AGSGravityManager> GravityManager = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    FGSGravityState CurrentGravityState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    FGSSurfaceModifier EffectiveSurfaceModifier;

    UPROPERTY(BlueprintAssignable, Category = "Gravity Shift|Events")
    FGSOnImpactGenerated OnImpactGenerated;

    UPROPERTY(BlueprintAssignable, Category = "Gravity Shift|Events")
    FGSOnEffectiveSurfaceChanged OnEffectiveSurfaceChanged;

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    bool ResolveTargetPrimitive();

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void SetGravityManager(AGSGravityManager* NewManager);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void ApplyGravityState(const FGSGravityState& NewState);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void SetRuntimeEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift|Surface")
    void PushSurfaceModifier(AActor* SourceActor, const FGSSurfaceModifier& Modifier);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift|Surface")
    void PopSurfaceModifier(AActor* SourceActor);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift|Surface")
    void ClearSurfaceModifiers();

    UFUNCTION(BlueprintPure, Category = "Gravity Shift")
    FVector GetGravityDirection() const;

    UFUNCTION(BlueprintPure, Category = "Gravity Shift")
    float GetSignedGravitySpeed() const;

private:
    UPROPERTY(Transient)
    TMap<AActor*, FGSSurfaceModifier> ActiveSurfaceModifiers;

    UPROPERTY(Transient)
    TMap<AActor*, int32> SurfaceModifierSerials;

    UPROPERTY(Transient)
    TMap<AActor*, float> RecentHitTimes;

    int32 NextSurfaceModifierSerial = 1;
    float TimeUntilManagerRetry = 0.0f;
    float TimeUntilMaintenance = 0.0f;
    FVector CachedPrePhysicsVelocity = FVector::ZeroVector;

    void TryFindManager();
    void RecalculateEffectiveSurface();
    void ApplyCustomGravity(float DeltaTime);
    void PruneTransientMaps();

    UFUNCTION()
    void HandleComponentHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);
};
