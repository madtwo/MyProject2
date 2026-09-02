#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityShiftTypes.h"
#include "GSBreakableComponent.generated.h"

class UGSBreakProfile;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FGSOnBreakableDamaged,
    float,
    Damage,
    float,
    RemainingHealth,
    FGSImpactPayload,
    Payload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FGSOnBreakableBroken,
    FGSImpactPayload,
    Payload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnBreakableRestored);

UCLASS(ClassGroup = (GravityShift), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GRAVITYSHIFTCORE_API UGSBreakableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGSBreakableComponent();

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Target")
    FName TargetComponentTag = TEXT("BreakableBody");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Target")
    bool bAllowPrimitiveFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable")
    bool bRuntimeEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable")
    TObjectPtr<UGSBreakProfile> BreakProfile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline", meta = (ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline", meta = (ClampMin = "0.0"))
    float MinEnergyJ = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline", meta = (ClampMin = "0.0"))
    float InstantBreakEnergyJ = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline", meta = (ClampMin = "0.0"))
    float DamagePerJoule = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline", meta = (ClampMin = "0.0"))
    float MaxDamagePerHit = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline")
    bool bInstantBreakAboveThreshold = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Inline")
    EGSDestructionMode DestructionMode = EGSDestructionMode::HideAndDisable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Visual")
    TObjectPtr<UStaticMesh> BrokenMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Visual")
    bool bDisablePhysicsOnBreak = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable|Visual")
    bool bDisableCollisionOnBreak = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breakable|State")
    TObjectPtr<UPrimitiveComponent> TargetPrimitive = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breakable|State")
    float CurrentHealth = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breakable|State")
    bool bBroken = false;

    UPROPERTY(BlueprintAssignable, Category = "Breakable|Events")
    FGSOnBreakableDamaged OnDamaged;

    UPROPERTY(BlueprintAssignable, Category = "Breakable|Events")
    FGSOnBreakableBroken OnBroken;

    UPROPERTY(BlueprintAssignable, Category = "Breakable|Events")
    FGSOnBreakableRestored OnRestored;

    UFUNCTION(BlueprintCallable, Category = "Breakable")
    bool ResolveTargetPrimitive();

    UFUNCTION(BlueprintCallable, Category = "Breakable")
    bool ReceiveImpact(const FGSImpactPayload& Payload);

    UFUNCTION(BlueprintCallable, Category = "Breakable")
    void BreakObject(const FGSImpactPayload& Payload);

    UFUNCTION(BlueprintCallable, Category = "Breakable")
    void RestoreBreakState();

    UFUNCTION(BlueprintCallable, Category = "Breakable")
    void SetRuntimeEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Breakable")
    float GetConfiguredMaxHealth() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Breakable", meta = (DisplayName = "On Broken FX"))
    void BP_OnBrokenFX(const FGSImpactPayload& Payload);

    UFUNCTION(BlueprintImplementableEvent, Category = "Breakable", meta = (DisplayName = "On Restored FX"))
    void BP_OnRestoredFX();

private:
    bool bInitialVisible = true;
    bool bInitialActorHidden = false;
    bool bInitialSimulatingPhysics = false;
    ECollisionEnabled::Type InitialCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> InitialStaticMesh = nullptr;

    float GetConfiguredMinEnergy() const;
    float GetConfiguredInstantBreakEnergy() const;
    float GetConfiguredDamagePerJoule() const;
    float GetConfiguredMaxDamagePerHit() const;
    bool GetConfiguredInstantBreakEnabled() const;
    EGSDestructionMode GetConfiguredDestructionMode() const;
};
