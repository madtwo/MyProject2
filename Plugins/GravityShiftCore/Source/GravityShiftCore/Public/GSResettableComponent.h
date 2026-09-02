#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityShiftTypes.h"
#include "GSResettableComponent.generated.h"

class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnResetSnapshotCaptured);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSOnResetStateRestored);

UCLASS(ClassGroup = (GravityShift), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GRAVITYSHIFTCORE_API UGSResettableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGSResettableComponent();

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset|Target")
    FName TargetComponentTag = TEXT("GravityBody");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset|Target")
    bool bAllowPrimitiveFallback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
    bool bCaptureOnBeginPlay = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reset")
    bool bHasSnapshot = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reset")
    FGSResetSnapshot Snapshot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reset")
    TObjectPtr<UPrimitiveComponent> TargetPrimitive = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Reset|Events")
    FGSOnResetSnapshotCaptured OnSnapshotCaptured;

    UPROPERTY(BlueprintAssignable, Category = "Reset|Events")
    FGSOnResetStateRestored OnStateRestored;

    UFUNCTION(BlueprintCallable, Category = "Reset")
    bool ResolveTargetPrimitive();

    UFUNCTION(BlueprintCallable, Category = "Reset")
    void CaptureResetSnapshot();

    UFUNCTION(BlueprintCallable, Category = "Reset")
    bool RestoreResetSnapshot();

private:
    bool bSnapshotSimulatingPhysics = false;
    ECollisionEnabled::Type SnapshotCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
};
