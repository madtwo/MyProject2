#include "GSGravityBodyComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GSBreakableComponent.h"
#include "GSGravityManager.h"
#include "GSGravityMathLibrary.h"
#include "GravityShiftCore.h"
#include "PhysicsEngine/BodyInstance.h"

UGSGravityBodyComponent::UGSGravityBodyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    CurrentGravityState.Axis = EGSGravityAxis::ZNegative;
    CurrentGravityState.Direction = FVector(0.0, 0.0, -1.0);
    CurrentGravityState.MagnitudeCms2 = 980.0f;
}

void UGSGravityBodyComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ResolveTargetPrimitive())
    {
        SetComponentTickEnabled(false);
        return;
    }

    TargetPrimitive->SetMobility(EComponentMobility::Movable);
    TargetPrimitive->SetEnableGravity(false);
    TargetPrimitive->SetNotifyRigidBodyCollision(true);
    TargetPrimitive->BodyInstance.bUseCCD = bUseCCD;

    if (bAutoEnablePhysics && !TargetPrimitive->IsSimulatingPhysics())
    {
        TargetPrimitive->SetSimulatePhysics(true);
    }

    TargetPrimitive->OnComponentHit.AddUniqueDynamic(
        this,
        &UGSGravityBodyComponent::HandleComponentHit);

    if (bAutoFindManager)
    {
        TryFindManager();
    }
}

void UGSGravityBodyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TargetPrimitive))
    {
        TargetPrimitive->OnComponentHit.RemoveDynamic(
            this,
            &UGSGravityBodyComponent::HandleComponentHit);
    }

    if (IsValid(GravityManager))
    {
        GravityManager->UnregisterGravityBody(this);
    }

    GravityManager = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UGSGravityBodyComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bRuntimeEnabled || !IsValid(TargetPrimitive))
    {
        return;
    }

    if (!IsValid(GravityManager) && bAutoFindManager)
    {
        TimeUntilManagerRetry -= DeltaTime;
        if (TimeUntilManagerRetry <= 0.0f)
        {
            TryFindManager();
            TimeUntilManagerRetry = ManagerRetrySeconds;
        }
    }

    CachedPrePhysicsVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
    ApplyCustomGravity(DeltaTime);

    TimeUntilMaintenance -= DeltaTime;
    if (TimeUntilMaintenance <= 0.0f)
    {
        PruneTransientMaps();
        TimeUntilMaintenance = 1.0f;
    }
}

bool UGSGravityBodyComponent::ResolveTargetPrimitive()
{
    TargetPrimitive = nullptr;

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    TArray<UPrimitiveComponent*> TaggedComponents;
    for (UPrimitiveComponent* Primitive : PrimitiveComponents)
    {
        if (IsValid(Primitive) && Primitive->ComponentHasTag(TargetComponentTag))
        {
            TaggedComponents.Add(Primitive);
        }
    }

    if (TaggedComponents.Num() == 1)
    {
        TargetPrimitive = TaggedComponents[0];
        return true;
    }

    if (TaggedComponents.Num() > 1)
    {
        TargetPrimitive = TaggedComponents[0];
        UE_LOG(
            LogGravityShift,
            Warning,
            TEXT("%s: multiple primitive components use tag '%s'. Using '%s'."),
            *Owner->GetName(),
            *TargetComponentTag.ToString(),
            *TargetPrimitive->GetName());
        return true;
    }

    if (!bAllowPrimitiveFallback)
    {
        UE_LOG(
            LogGravityShift,
            Error,
            TEXT("%s: no primitive component uses tag '%s'."),
            *Owner->GetName(),
            *TargetComponentTag.ToString());
        return false;
    }

    for (UPrimitiveComponent* Primitive : PrimitiveComponents)
    {
        if (IsValid(Primitive) && Primitive->IsSimulatingPhysics())
        {
            TargetPrimitive = Primitive;
            break;
        }
    }

    if (!TargetPrimitive)
    {
        TargetPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    }

    if (!TargetPrimitive && !PrimitiveComponents.IsEmpty())
    {
        TargetPrimitive = PrimitiveComponents[0];
    }

    if (!TargetPrimitive)
    {
        UE_LOG(
            LogGravityShift,
            Error,
            TEXT("%s: no primitive component was found for custom gravity."),
            *Owner->GetName());
        return false;
    }

    UE_LOG(
        LogGravityShift,
        Warning,
        TEXT("%s: tag '%s' was not found. Falling back to '%s'."),
        *Owner->GetName(),
        *TargetComponentTag.ToString(),
        *TargetPrimitive->GetName());

    return true;
}

void UGSGravityBodyComponent::SetGravityManager(AGSGravityManager* NewManager)
{
    if (GravityManager == NewManager)
    {
        return;
    }

    if (IsValid(GravityManager))
    {
        GravityManager->UnregisterGravityBody(this);
    }

    GravityManager = NewManager;

    if (IsValid(GravityManager))
    {
        GravityManager->RegisterGravityBody(this);
        ApplyGravityState(GravityManager->GetGravityState());
    }
}

void UGSGravityBodyComponent::ApplyGravityState(const FGSGravityState& NewState)
{
    CurrentGravityState = NewState;
    CurrentGravityState.Direction = CurrentGravityState.Direction.GetSafeNormal();

    if (IsValid(TargetPrimitive) && TargetPrimitive->IsSimulatingPhysics())
    {
        const FVector ExistingVelocity = TargetPrimitive->GetPhysicsLinearVelocity();
        TargetPrimitive->SetPhysicsLinearVelocity(
            ExistingVelocity * FMath::Max(0.0f, VelocityRetentionOnShift),
            false);
        TargetPrimitive->WakeAllRigidBodies();
    }
}

void UGSGravityBodyComponent::SetRuntimeEnabled(const bool bEnabled)
{
    bRuntimeEnabled = bEnabled;

    if (IsValid(TargetPrimitive))
    {
        TargetPrimitive->SetEnableGravity(false);
        if (bRuntimeEnabled)
        {
            TargetPrimitive->WakeAllRigidBodies();
        }
    }

    SetComponentTickEnabled(bRuntimeEnabled);
}

void UGSGravityBodyComponent::PushSurfaceModifier(
    AActor* SourceActor,
    const FGSSurfaceModifier& Modifier)
{
    if (!IsValid(SourceActor))
    {
        return;
    }

    ActiveSurfaceModifiers.Add(SourceActor, Modifier);
    SurfaceModifierSerials.Add(SourceActor, NextSurfaceModifierSerial++);
    RecalculateEffectiveSurface();
}

void UGSGravityBodyComponent::PopSurfaceModifier(AActor* SourceActor)
{
    if (!SourceActor)
    {
        return;
    }

    ActiveSurfaceModifiers.Remove(SourceActor);
    SurfaceModifierSerials.Remove(SourceActor);
    RecalculateEffectiveSurface();
}

void UGSGravityBodyComponent::ClearSurfaceModifiers()
{
    ActiveSurfaceModifiers.Reset();
    SurfaceModifierSerials.Reset();
    RecalculateEffectiveSurface();
}

FVector UGSGravityBodyComponent::GetGravityDirection() const
{
    return CurrentGravityState.Direction.GetSafeNormal();
}

float UGSGravityBodyComponent::GetSignedGravitySpeed() const
{
    if (!IsValid(TargetPrimitive))
    {
        return 0.0f;
    }

    return FVector::DotProduct(
        TargetPrimitive->GetPhysicsLinearVelocity(),
        GetGravityDirection());
}

void UGSGravityBodyComponent::TryFindManager()
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AGSGravityManager> It(GetWorld()); It; ++It)
    {
        SetGravityManager(*It);
        return;
    }
}

void UGSGravityBodyComponent::RecalculateEffectiveSurface()
{
    FGSSurfaceModifier SelectedModifier;
    int32 SelectedPriority = TNumericLimits<int32>::Lowest();
    int32 SelectedSerial = TNumericLimits<int32>::Lowest();

    for (const TPair<AActor*, FGSSurfaceModifier>& Pair : ActiveSurfaceModifiers)
    {
        AActor* Source = Pair.Key;
        if (!IsValid(Source))
        {
            continue;
        }

        const int32 Serial = SurfaceModifierSerials.FindRef(Source);
        const bool bHigherPriority = Pair.Value.Priority > SelectedPriority;
        const bool bNewerAtSamePriority =
            Pair.Value.Priority == SelectedPriority && Serial > SelectedSerial;

        if (bHigherPriority || bNewerAtSamePriority)
        {
            SelectedModifier = Pair.Value;
            SelectedPriority = Pair.Value.Priority;
            SelectedSerial = Serial;
        }
    }

    EffectiveSurfaceModifier = SelectedModifier;
    OnEffectiveSurfaceChanged.Broadcast(EffectiveSurfaceModifier);
}

void UGSGravityBodyComponent::ApplyCustomGravity(const float DeltaTime)
{
    if (!TargetPrimitive->IsSimulatingPhysics())
    {
        return;
    }

    FVector GravityDirection = CurrentGravityState.Direction.GetSafeNormal();
    if (GravityDirection.IsNearlyZero())
    {
        GravityDirection = FVector(0.0, 0.0, -1.0);
    }

    const float GravityMultiplier = FMath::Max(0.0f, EffectiveSurfaceModifier.GravityMultiplier);
    const FVector GravityAcceleration =
        GravityDirection *
        CurrentGravityState.MagnitudeCms2 *
        FMath::Max(0.0f, GravityScale) *
        GravityMultiplier;

    TargetPrimitive->AddForce(GravityAcceleration, NAME_None, true);

    const FVector Velocity = TargetPrimitive->GetPhysicsLinearVelocity();
    const float SignedGravitySpeed = FVector::DotProduct(Velocity, GravityDirection);
    const float FallingGravitySpeed = FMath::Max(0.0f, SignedGravitySpeed);
    const FVector TangentialVelocity =
        Velocity - GravityDirection * SignedGravitySpeed;

    const FVector GravityDragAcceleration =
        -GravityDirection *
        FallingGravitySpeed *
        FMath::Max(0.0f, EffectiveSurfaceModifier.GravityDragPerSecond);

    const FVector TangentialDragAcceleration =
        -TangentialVelocity *
        FMath::Max(0.0f, EffectiveSurfaceModifier.TangentialDragPerSecond);

    const FVector TotalDragAcceleration =
        GravityDragAcceleration + TangentialDragAcceleration;

    if (!TotalDragAcceleration.IsNearlyZero())
    {
        TargetPrimitive->AddForce(TotalDragAcceleration, NAME_None, true);
    }

    float EffectiveTerminalSpeed = BaseTerminalSpeedCms;
    if (EffectiveSurfaceModifier.TerminalGravitySpeedCms > 0.0f)
    {
        EffectiveTerminalSpeed =
            EffectiveTerminalSpeed > 0.0f
            ? FMath::Min(EffectiveTerminalSpeed, EffectiveSurfaceModifier.TerminalGravitySpeedCms)
            : EffectiveSurfaceModifier.TerminalGravitySpeedCms;
    }

    if (EffectiveTerminalSpeed > 0.0f && FallingGravitySpeed > EffectiveTerminalSpeed)
    {
        const FVector ClampedVelocity =
            TangentialVelocity + GravityDirection * EffectiveTerminalSpeed;
        TargetPrimitive->SetPhysicsLinearVelocity(ClampedVelocity, false);
    }
}

void UGSGravityBodyComponent::PruneTransientMaps()
{
    for (auto It = ActiveSurfaceModifiers.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()))
        {
            SurfaceModifierSerials.Remove(It.Key());
            It.RemoveCurrent();
        }
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    for (auto It = RecentHitTimes.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()) || Now - It.Value() > FMath::Max(1.0f, HitDebounceSeconds * 4.0f))
        {
            It.RemoveCurrent();
        }
    }
}

void UGSGravityBodyComponent::HandleComponentHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!bCanDealImpact ||
        !IsValid(TargetPrimitive) ||
        !IsValid(OtherActor) ||
        OtherActor == GetOwner())
    {
        return;
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (const float* PreviousTime = RecentHitTimes.Find(OtherActor))
    {
        if (Now - *PreviousTime < HitDebounceSeconds)
        {
            return;
        }
    }
    RecentHitTimes.Add(OtherActor, Now);

    const FVector SelfVelocity =
        CachedPrePhysicsVelocity.IsNearlyZero()
        ? TargetPrimitive->GetPhysicsLinearVelocity()
        : CachedPrePhysicsVelocity;

    FVector OtherVelocity = FVector::ZeroVector;
    if (IsValid(OtherComponent))
    {
        OtherVelocity =
            OtherComponent->IsSimulatingPhysics()
            ? OtherComponent->GetPhysicsLinearVelocity()
            : OtherComponent->GetComponentVelocity();
    }

    FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    if (ImpactNormal.IsNearlyZero())
    {
        ImpactNormal = Hit.Normal.GetSafeNormal();
    }
    if (ImpactNormal.IsNearlyZero())
    {
        ImpactNormal = FVector::UpVector;
    }

    const FVector RelativeVelocity = SelfVelocity - OtherVelocity;
    const float RelativeNormalSpeedCms =
        FMath::Abs(FVector::DotProduct(RelativeVelocity, ImpactNormal));

    const float MassKg = FMath::Max(0.001f, TargetPrimitive->GetMass());
    const float SurfaceMultiplier =
        FMath::Max(0.0f, EffectiveSurfaceModifier.ImpactEnergyMultiplier);
    const float EnergyJ =
        UGSGravityMathLibrary::ComputeImpactEnergyJ(MassKg, RelativeNormalSpeedCms) *
        FMath::Max(0.0f, ImpactEnergyMultiplier) *
        SurfaceMultiplier;

    FGSImpactPayload Payload;
    Payload.SourceActor = GetOwner();
    Payload.SourceComponent = TargetPrimitive;
    Payload.HitActor = OtherActor;
    Payload.HitComponent = OtherComponent;
    Payload.HitLocation = Hit.ImpactPoint;
    Payload.HitNormal = ImpactNormal;
    Payload.RelativeNormalSpeedCms = RelativeNormalSpeedCms;
    Payload.SourceMassKg = MassKg;
    Payload.EnergyJ = EnergyJ;
    Payload.ImpactTier = UGSGravityMathLibrary::SelectImpactTier(
        EnergyJ,
        LightEnergyJ,
        HeavyEnergyJ,
        CriticalEnergyJ);
    Payload.bHasBreakPower = EnergyJ >= BreakPowerMinEnergyJ;
    Payload.GravityRevision = CurrentGravityState.Revision;

    OnImpactGenerated.Broadcast(Payload);

    TArray<UGSBreakableComponent*> BreakableComponents;
    OtherActor->GetComponents<UGSBreakableComponent>(BreakableComponents);
    for (UGSBreakableComponent* Breakable : BreakableComponents)
    {
        if (IsValid(Breakable))
        {
            Breakable->ReceiveImpact(Payload);
        }
    }
}
