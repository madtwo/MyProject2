#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityShiftTypes.h"
#include "GSSurfaceModifierVolume.generated.h"

class USceneComponent;
class UBoxComponent;
class UGSSurfaceProfile;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGSOnSurfaceProfileChanged,
    bool,
    bUsingProfileB,
    FGSSurfaceModifier,
    ActiveModifier);

UCLASS(BlueprintType, Blueprintable)
class GRAVITYSHIFTCORE_API AGSSurfaceModifierVolume : public AActor
{
    GENERATED_BODY()

public:
    AGSSurfaceModifierVolume();

    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> SurfaceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> InfluenceVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    TObjectPtr<UGSSurfaceProfile> ProfileA = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    TObjectPtr<UGSSurfaceProfile> ProfileB = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FGSSurfaceModifier InlineProfileA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FGSSurfaceModifier InlineProfileB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    bool bUseProfileB = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    bool bCanBeToggled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    int32 UsesRemaining = -1;

    UPROPERTY(BlueprintAssignable, Category = "Surface|Events")
    FGSOnSurfaceProfileChanged OnSurfaceProfileChanged;

    UFUNCTION(BlueprintCallable, Category = "Surface")
    bool ToggleProfile();

    UFUNCTION(BlueprintCallable, Category = "Surface")
    void SetUseProfileB(bool bNewUseProfileB);

    UFUNCTION(BlueprintCallable, Category = "Surface")
    void RefreshOverlappingActors();

    UFUNCTION(BlueprintPure, Category = "Surface")
    FGSSurfaceModifier GetActiveModifier() const;

private:
    UPROPERTY(Transient)
    TSet<AActor*> TrackedOverlappingActors;

    UFUNCTION()
    void HandleBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex);

    void ApplyModifierToActor(AActor* TargetActor);
    void RemoveModifierFromActor(AActor* TargetActor);
    void PruneTrackedActors();
};
