#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "GravityShiftTypes.h"
#include "GSGravityManager.generated.h"

class UGSGravityBodyComponent;
class UGSGravityRuleSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGSOnGravityAboutToChange,
    FGSGravityState,
    OldState,
    FGSGravityState,
    NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FGSOnGravityChanged,
    FGSGravityState,
    NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGSOnGravityRejected,
    EGSGravityRequestResult,
    Result,
    EGSGravityAxis,
    RequestedAxis);

UCLASS(BlueprintType, Blueprintable)
class GRAVITYSHIFTCORE_API AGSGravityManager : public AActor
{
    GENERATED_BODY()

public:
    AGSGravityManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Shift|Rules")
    TObjectPtr<UGSGravityRuleSet> RuleSet = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Rules")
    TArray<EGSGravityAxis> FallbackAllowedAxes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Rules", meta = (ClampMin = "0.0", Units = "cm/s^2"))
    float FallbackGravityMagnitudeCms2 = 980.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Rules", meta = (ClampMin = "0.0", Units = "s"))
    float FallbackManualCooldown = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Rules", meta = (ClampMin = "0.0", Units = "s"))
    float FallbackAutoReverseCooldown = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Debug")
    bool bEnableDebugKeyInput = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Debug")
    FKey DebugNextGravityKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift|Debug")
    FKey DebugResetKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    FGSGravityState CurrentState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    float NextManualChangeTime = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    float NextAutoReverseTime = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity Shift|State")
    bool bTransitionBusy = false;

    UPROPERTY(BlueprintAssignable, Category = "Gravity Shift|Events")
    FGSOnGravityAboutToChange OnGravityAboutToChange;

    UPROPERTY(BlueprintAssignable, Category = "Gravity Shift|Events")
    FGSOnGravityChanged OnGravityChanged;

    UPROPERTY(BlueprintAssignable, Category = "Gravity Shift|Events")
    FGSOnGravityRejected OnGravityRejected;

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    EGSGravityRequestResult RequestNextGravity(
        AActor* InstigatorActor,
        EGSGravityChangeReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    EGSGravityRequestResult RequestGravityAxis(
        EGSGravityAxis Axis,
        AActor* InstigatorActor,
        EGSGravityChangeReason Reason,
        bool bIgnoreCooldown);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void CommitGravityChange(
        EGSGravityAxis Axis,
        AActor* InstigatorActor,
        EGSGravityChangeReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void ResetToInitialGravity();

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void ResetPuzzleState();

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void RegisterGravityBody(UGSGravityBodyComponent* GravityBody);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void UnregisterGravityBody(UGSGravityBodyComponent* GravityBody);

    UFUNCTION(BlueprintCallable, Category = "Gravity Shift")
    void PruneInvalidRegistrations();

    UFUNCTION(BlueprintPure, Category = "Gravity Shift")
    FGSGravityState GetGravityState() const;

    UFUNCTION(BlueprintPure, Category = "Gravity Shift")
    FVector GetGravityDirection() const;

    UFUNCTION(BlueprintPure, Category = "Gravity Shift")
    TArray<EGSGravityAxis> GetAllowedAxes() const;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UGSGravityBodyComponent>> RegisteredGravityBodies;

    UPROPERTY(Transient)
    TObjectPtr<AActor> QueuedInstigator = nullptr;

    EGSGravityAxis QueuedAxis = EGSGravityAxis::ZNegative;
    EGSGravityChangeReason QueuedReason = EGSGravityChangeReason::Scripted;
    bool bHasQueuedRequest = false;
    bool bHasBegunPlay = false;

    bool bWasNextGravityKeyDown = false;
    bool bWasResetKeyDown = false;

    UFUNCTION()
    void HandleDebugNextGravity();

    UFUNCTION()
    void HandleDebugReset();

    void RefreshSurfaceVolumesAfterReset();

    EGSGravityAxis GetInitialAxis() const;
    float GetGravityMagnitude() const;
    float GetManualCooldown() const;
    float GetAutoReverseCooldown() const;
    bool IsAxisAllowed(EGSGravityAxis Axis) const;
    void InitializeState();
    void ApplyCurrentStateToRegisteredBodies();
    void BroadcastRejected(EGSGravityRequestResult Result, EGSGravityAxis RequestedAxis);
};
