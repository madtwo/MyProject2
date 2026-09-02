#include "GSResettableComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GSBreakableComponent.h"
#include "GravityShiftCore.h"

UGSResettableComponent::UGSResettableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGSResettableComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveTargetPrimitive();

    if (bCaptureOnBeginPlay)
    {
        CaptureResetSnapshot();
    }
}

bool UGSResettableComponent::ResolveTargetPrimitive()
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
        return false;
    }

    TargetPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!TargetPrimitive && !PrimitiveComponents.IsEmpty())
    {
        TargetPrimitive = PrimitiveComponents[0];
    }

    return IsValid(TargetPrimitive);
}

void UGSResettableComponent::CaptureResetSnapshot()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    Snapshot.Transform = Owner->GetActorTransform();
    Snapshot.bHidden = Owner->IsHidden();

    if (!IsValid(TargetPrimitive))
    {
        ResolveTargetPrimitive();
    }

    if (IsValid(TargetPrimitive))
    {
        Snapshot.LinearVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
        Snapshot.AngularVelocityDeg = TargetPrimitive->GetPhysicsAngularVelocityInDegrees();
        SnapshotCollisionEnabled = TargetPrimitive->GetCollisionEnabled();
        Snapshot.bCollisionEnabled = SnapshotCollisionEnabled != ECollisionEnabled::NoCollision;
        bSnapshotSimulatingPhysics = TargetPrimitive->IsSimulatingPhysics();
    }
    else
    {
        Snapshot.LinearVelocity = FVector::ZeroVector;
        Snapshot.AngularVelocityDeg = FVector::ZeroVector;
        Snapshot.bCollisionEnabled = false;
        SnapshotCollisionEnabled = ECollisionEnabled::NoCollision;
        bSnapshotSimulatingPhysics = false;
    }

    bHasSnapshot = true;
    OnSnapshotCaptured.Broadcast();
}

bool UGSResettableComponent::RestoreResetSnapshot()
{
    AActor* Owner = GetOwner();
    if (!bHasSnapshot || !IsValid(Owner))
    {
        return false;
    }

    if (!IsValid(TargetPrimitive))
    {
        ResolveTargetPrimitive();
    }

    if (IsValid(TargetPrimitive) && TargetPrimitive->IsSimulatingPhysics())
    {
        TargetPrimitive->SetSimulatePhysics(false);
    }

    Owner->SetActorTransform(
        Snapshot.Transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    Owner->SetActorHiddenInGame(Snapshot.bHidden);

    if (UGSBreakableComponent* Breakable = Owner->FindComponentByClass<UGSBreakableComponent>())
    {
        Breakable->RestoreBreakState();
    }

    if (IsValid(TargetPrimitive))
    {
        TargetPrimitive->SetCollisionEnabled(SnapshotCollisionEnabled);
        TargetPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
        TargetPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
        TargetPrimitive->SetSimulatePhysics(bSnapshotSimulatingPhysics);

        if (bSnapshotSimulatingPhysics)
        {
            TargetPrimitive->SetPhysicsLinearVelocity(Snapshot.LinearVelocity, false);
            TargetPrimitive->SetPhysicsAngularVelocityInDegrees(
                Snapshot.AngularVelocityDeg,
                false);
            TargetPrimitive->WakeAllRigidBodies();
        }
    }

    OnStateRestored.Broadcast();
    return true;
}
