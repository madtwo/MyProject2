#include "GSBreakableComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GravityShiftCore.h"
#include "GravityShiftProfiles.h"

UGSBreakableComponent::UGSBreakableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGSBreakableComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveTargetPrimitive();
    CurrentHealth = GetConfiguredMaxHealth();

    if (IsValid(TargetPrimitive))
    {
        bInitialVisible = TargetPrimitive->IsVisible();
        bInitialSimulatingPhysics = TargetPrimitive->IsSimulatingPhysics();
        InitialCollisionEnabled = TargetPrimitive->GetCollisionEnabled();

        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(TargetPrimitive))
        {
            InitialStaticMesh = StaticMeshComponent->GetStaticMesh();
        }
    }

    if (AActor* Owner = GetOwner())
    {
        bInitialActorHidden = Owner->IsHidden();
    }
}

bool UGSBreakableComponent::ResolveTargetPrimitive()
{
    TargetPrimitive = nullptr;

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* Primitive : PrimitiveComponents)
    {
        if (IsValid(Primitive) && Primitive->ComponentHasTag(TargetComponentTag))
        {
            TargetPrimitive = Primitive;
            return true;
        }
    }

    if (!bAllowPrimitiveFallback)
    {
        UE_LOG(
            LogGravityShift,
            Error,
            TEXT("%s: Breakable component cannot find target tag '%s'."),
            *Owner->GetName(),
            *TargetComponentTag.ToString());
        return false;
    }

    TargetPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!TargetPrimitive && !PrimitiveComponents.IsEmpty())
    {
        TargetPrimitive = PrimitiveComponents[0];
    }

    if (!TargetPrimitive)
    {
        UE_LOG(
            LogGravityShift,
            Error,
            TEXT("%s: Breakable component cannot find any primitive component."),
            *Owner->GetName());
        return false;
    }

    return true;
}

bool UGSBreakableComponent::ReceiveImpact(const FGSImpactPayload& Payload)
{
    if (!bRuntimeEnabled || bBroken)
    {
        return false;
    }

    const float RequiredEnergy = GetConfiguredMinEnergy();
    if (Payload.EnergyJ < RequiredEnergy)
    {
        return false;
    }

    if (GetConfiguredInstantBreakEnabled() &&
        Payload.EnergyJ >= GetConfiguredInstantBreakEnergy())
    {
        BreakObject(Payload);
        return true;
    }

    const float Damage = FMath::Clamp(
        (Payload.EnergyJ - RequiredEnergy) * GetConfiguredDamagePerJoule(),
        0.0f,
        GetConfiguredMaxDamagePerHit());

    if (Damage <= 0.0f)
    {
        return false;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - Damage);
    OnDamaged.Broadcast(Damage, CurrentHealth, Payload);

    if (CurrentHealth <= 0.0f)
    {
        BreakObject(Payload);
    }

    return true;
}

void UGSBreakableComponent::BreakObject(const FGSImpactPayload& Payload)
{
    if (!bRuntimeEnabled || bBroken)
    {
        return;
    }

    bBroken = true;
    CurrentHealth = 0.0f;

    if (!IsValid(TargetPrimitive))
    {
        ResolveTargetPrimitive();
    }

    if (IsValid(TargetPrimitive))
    {
        switch (GetConfiguredDestructionMode())
        {
            case EGSDestructionMode::SwapMesh:
            {
                if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(TargetPrimitive))
                {
                    if (IsValid(BrokenMesh))
                    {
                        StaticMeshComponent->SetStaticMesh(BrokenMesh);
                    }
                    else
                    {
                        TargetPrimitive->SetVisibility(false, true);
                    }
                }
                else
                {
                    TargetPrimitive->SetVisibility(false, true);
                }
                break;
            }

            case EGSDestructionMode::GeometryCollectionOptional:
            case EGSDestructionMode::HideAndDisable:
            default:
                TargetPrimitive->SetVisibility(false, true);
                break;
        }

        if (bDisableCollisionOnBreak)
        {
            TargetPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        if (bDisablePhysicsOnBreak && TargetPrimitive->IsSimulatingPhysics())
        {
            TargetPrimitive->SetSimulatePhysics(false);
        }
    }

    OnBroken.Broadcast(Payload);
    BP_OnBrokenFX(Payload);
}

void UGSBreakableComponent::RestoreBreakState()
{
    bBroken = false;
    CurrentHealth = GetConfiguredMaxHealth();

    if (!IsValid(TargetPrimitive))
    {
        ResolveTargetPrimitive();
    }

    if (AActor* Owner = GetOwner())
    {
        Owner->SetActorHiddenInGame(bInitialActorHidden);
    }

    if (IsValid(TargetPrimitive))
    {
        if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(TargetPrimitive))
        {
            if (IsValid(InitialStaticMesh))
            {
                StaticMeshComponent->SetStaticMesh(InitialStaticMesh);
            }
        }

        TargetPrimitive->SetVisibility(bInitialVisible, true);
        TargetPrimitive->SetCollisionEnabled(InitialCollisionEnabled);
        TargetPrimitive->SetSimulatePhysics(bInitialSimulatingPhysics);

        if (bInitialSimulatingPhysics)
        {
            TargetPrimitive->WakeAllRigidBodies();
        }
    }

    OnRestored.Broadcast();
    BP_OnRestoredFX();
}

void UGSBreakableComponent::SetRuntimeEnabled(const bool bEnabled)
{
    bRuntimeEnabled = bEnabled;

    if (!bRuntimeEnabled && bBroken)
    {
        RestoreBreakState();
    }
}

float UGSBreakableComponent::GetConfiguredMaxHealth() const
{
    return BreakProfile
        ? FMath::Max(1.0f, BreakProfile->MaxHealth)
        : FMath::Max(1.0f, MaxHealth);
}

float UGSBreakableComponent::GetConfiguredMinEnergy() const
{
    return BreakProfile
        ? FMath::Max(0.0f, BreakProfile->MinEnergyJ)
        : FMath::Max(0.0f, MinEnergyJ);
}

float UGSBreakableComponent::GetConfiguredInstantBreakEnergy() const
{
    return BreakProfile
        ? FMath::Max(0.0f, BreakProfile->InstantBreakEnergyJ)
        : FMath::Max(0.0f, InstantBreakEnergyJ);
}

float UGSBreakableComponent::GetConfiguredDamagePerJoule() const
{
    return BreakProfile
        ? FMath::Max(0.0f, BreakProfile->DamagePerJoule)
        : FMath::Max(0.0f, DamagePerJoule);
}

float UGSBreakableComponent::GetConfiguredMaxDamagePerHit() const
{
    return BreakProfile
        ? FMath::Max(0.0f, BreakProfile->MaxDamagePerHit)
        : FMath::Max(0.0f, MaxDamagePerHit);
}

bool UGSBreakableComponent::GetConfiguredInstantBreakEnabled() const
{
    return BreakProfile
        ? BreakProfile->bInstantBreakAboveThreshold
        : bInstantBreakAboveThreshold;
}

EGSDestructionMode UGSBreakableComponent::GetConfiguredDestructionMode() const
{
    return BreakProfile
        ? BreakProfile->DestructionMode
        : DestructionMode;
}
