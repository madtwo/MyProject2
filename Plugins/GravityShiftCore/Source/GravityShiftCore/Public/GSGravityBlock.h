#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSGravityBlock.generated.h"

class UGSBlockProfile;
class UGSBreakableComponent;
class UGSGravityBodyComponent;
class UGSResettableComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class GRAVITYSHIFTCORE_API AGSGravityBlock : public AActor
{
    GENERATED_BODY()

public:
    AGSGravityBlock();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGSGravityBodyComponent> GravityBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGSBreakableComponent> Breakable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGSResettableComponent> Resettable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
    TObjectPtr<UGSBlockProfile> BlockProfile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline")
    bool bSimulatePhysics = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline")
    bool bGravityAffected = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline")
    bool bBreakable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline", meta = (ClampMin = "0.001", Units = "kg"))
    float MassOverrideKg = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline", meta = (ClampMin = "0.0"))
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block|Inline")
    bool bUseCCD = true;

    UFUNCTION(BlueprintCallable, Category = "Block")
    void ApplyBlockProfile();

    UFUNCTION(BlueprintCallable, Category = "Block")
    void ResetBlock();

private:
    void ApplyBlockProfileInternal(bool bApplyRuntimePhysics);
};
