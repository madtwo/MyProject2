#include "GSGravityManager.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GSGravityBodyComponent.h"
#include "GSGravityMathLibrary.h"
#include "GSResettableComponent.h"
#include "GSSurfaceModifierVolume.h"
#include "GravityShiftCore.h"
#include "GravityShiftProfiles.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGSGravityManager::AGSGravityManager()
{
    PrimaryActorTick.bCanEverTick = true;
    DebugNextGravityKey = EKeys::G;
    DebugResetKey = EKeys::R;

    FallbackAllowedAxes =
    {
        EGSGravityAxis::ZNegative,
        EGSGravityAxis::XPositive,
        EGSGravityAxis::ZPositive,
        EGSGravityAxis::XNegative,
        EGSGravityAxis::YPositive,
        EGSGravityAxis::YNegative
    };
}

void AGSGravityManager::BeginPlay()
{
    Super::BeginPlay();

    InitializeState();
    bHasBegunPlay = true;

    int32 ManagerCount = 0;
    for (TActorIterator<AGSGravityManager> It(GetWorld()); It; ++It)
    {
        ++ManagerCount;
    }

    if (ManagerCount > 1)
    {
        UE_LOG(
            LogGravityShift,
            Warning,
            TEXT("%s: %d Gravity Manager actors exist in this world. Use exactly one."),
            *GetName(),
            ManagerCount);
    }

    ApplyCurrentStateToRegisteredBodies();

    // Debug keys are polled in Tick via PlayerInput key state (IsInputKeyDown) instead of
    // InputComponent BindKey: an EnableInput push done during BeginPlay does not reliably
    // stay on the player controller's input stack for level-placed instances, so bound
    // keys silently never fire. Raw key state is available as soon as the controller exists.
    if (bEnableDebugKeyInput)
    {
        UE_LOG(
            LogGravityShift,
            Log,
            TEXT("%s: debug key polling active. NextGravity=%s, Reset=%s"),
            *GetName(),
            *DebugNextGravityKey.GetFName().ToString(),
            *DebugResetKey.GetFName().ToString());
    }

    OnGravityChanged.Broadcast(CurrentState);
}

void AGSGravityManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bEnableDebugKeyInput)
    {
        return;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        return;
    }

    const bool bNextDown = PlayerController->IsInputKeyDown(DebugNextGravityKey);
    const bool bResetDown = PlayerController->IsInputKeyDown(DebugResetKey);

    if (bNextDown && !bWasNextGravityKeyDown)
    {
        HandleDebugNextGravity();
    }
    if (bResetDown && !bWasResetKeyDown)
    {
        HandleDebugReset();
    }

    bWasNextGravityKeyDown = bNextDown;
    bWasResetKeyDown = bResetDown;
}

void AGSGravityManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RegisteredGravityBodies.Reset();
    Super::EndPlay(EndPlayReason);
}

EGSGravityRequestResult AGSGravityManager::RequestNextGravity(
    AActor* InstigatorActor,
    const EGSGravityChangeReason Reason)
{
    const TArray<EGSGravityAxis> AllowedAxes = GetAllowedAxes();
    if (AllowedAxes.IsEmpty())
    {
        BroadcastRejected(EGSGravityRequestResult::DirectionNotAllowed, CurrentState.Axis);
        return EGSGravityRequestResult::DirectionNotAllowed;
    }

    int32 CurrentIndex = AllowedAxes.IndexOfByKey(CurrentState.Axis);
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }
    else
    {
        CurrentIndex = (CurrentIndex + 1) % AllowedAxes.Num();
    }

    return RequestGravityAxis(AllowedAxes[CurrentIndex], InstigatorActor, Reason, false);
}

EGSGravityRequestResult AGSGravityManager::RequestGravityAxis(
    const EGSGravityAxis Axis,
    AActor* InstigatorActor,
    const EGSGravityChangeReason Reason,
    const bool bIgnoreCooldown)
{
    const FVector RequestedDirection = UGSGravityMathLibrary::AxisToDirection(Axis);
    if (!UGSGravityMathLibrary::IsCardinalUnitVector(RequestedDirection, 0.001f))
    {
        BroadcastRejected(EGSGravityRequestResult::InvalidDirection, Axis);
        return EGSGravityRequestResult::InvalidDirection;
    }

    if (!IsAxisAllowed(Axis))
    {
        BroadcastRejected(EGSGravityRequestResult::DirectionNotAllowed, Axis);
        return EGSGravityRequestResult::DirectionNotAllowed;
    }

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (!bIgnoreCooldown)
    {
        if (Reason == EGSGravityChangeReason::PlayerInput && Now < NextManualChangeTime)
        {
            BroadcastRejected(EGSGravityRequestResult::Cooldown, Axis);
            return EGSGravityRequestResult::Cooldown;
        }

        if (Reason == EGSGravityChangeReason::LandingAutoReverse && Now < NextAutoReverseTime)
        {
            BroadcastRejected(EGSGravityRequestResult::Cooldown, Axis);
            return EGSGravityRequestResult::Cooldown;
        }
    }

    if (bTransitionBusy)
    {
        QueuedAxis = Axis;
        QueuedInstigator = InstigatorActor;
        QueuedReason = Reason;
        bHasQueuedRequest = true;
        BroadcastRejected(EGSGravityRequestResult::TransitionBusy, Axis);
        return EGSGravityRequestResult::TransitionBusy;
    }

    CommitGravityChange(Axis, InstigatorActor, Reason);
    return EGSGravityRequestResult::Accepted;
}

void AGSGravityManager::CommitGravityChange(
    const EGSGravityAxis Axis,
    AActor* InstigatorActor,
    const EGSGravityChangeReason Reason)
{
    if (bTransitionBusy)
    {
        QueuedAxis = Axis;
        QueuedInstigator = InstigatorActor;
        QueuedReason = Reason;
        bHasQueuedRequest = true;
        return;
    }

    bTransitionBusy = true;

    const FGSGravityState OldState = CurrentState;
    FGSGravityState NewState;
    NewState.Axis = Axis;
    NewState.Direction = UGSGravityMathLibrary::AxisToDirection(Axis).GetSafeNormal();
    NewState.MagnitudeCms2 = GetGravityMagnitude();
    NewState.Revision = OldState.Revision + 1;
    NewState.Reason = Reason;
    NewState.InstigatorActor = InstigatorActor;
    NewState.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    OnGravityAboutToChange.Broadcast(OldState, NewState);
    CurrentState = NewState;

    if (Reason == EGSGravityChangeReason::PlayerInput)
    {
        NextManualChangeTime = CurrentState.WorldTimeSeconds + GetManualCooldown();
    }
    else if (Reason == EGSGravityChangeReason::LandingAutoReverse)
    {
        NextAutoReverseTime = CurrentState.WorldTimeSeconds + GetAutoReverseCooldown();
    }

    ApplyCurrentStateToRegisteredBodies();
    OnGravityChanged.Broadcast(CurrentState);

    bTransitionBusy = false;

    if (bHasQueuedRequest)
    {
        const EGSGravityAxis PendingAxis = QueuedAxis;
        AActor* PendingInstigator = QueuedInstigator.Get();
        const EGSGravityChangeReason PendingReason = QueuedReason;
        bHasQueuedRequest = false;
        QueuedInstigator = nullptr;

        RequestGravityAxis(
            PendingAxis,
            PendingInstigator,
            PendingReason,
            true);
    }
}

void AGSGravityManager::ResetToInitialGravity()
{
    NextManualChangeTime = 0.0f;
    NextAutoReverseTime = 0.0f;
    bHasQueuedRequest = false;
    QueuedInstigator = nullptr;
    CommitGravityChange(GetInitialAxis(), this, EGSGravityChangeReason::Reset);
}


void AGSGravityManager::ResetPuzzleState()
{
    ResetToInitialGravity();
    PruneInvalidRegistrations();

    TSet<AActor*> RestoredActors;
    for (UGSGravityBodyComponent* Body : RegisteredGravityBodies)
    {
        if (!IsValid(Body))
        {
            continue;
        }

        Body->ClearSurfaceModifiers();

        AActor* OwnerActor = Body->GetOwner();
        if (!IsValid(OwnerActor) || RestoredActors.Contains(OwnerActor))
        {
            continue;
        }

        RestoredActors.Add(OwnerActor);
        if (UGSResettableComponent* Resettable =
            OwnerActor->FindComponentByClass<UGSResettableComponent>())
        {
            Resettable->RestoreResetSnapshot();
        }
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(
                this,
                &AGSGravityManager::RefreshSurfaceVolumesAfterReset));
    }
}

void AGSGravityManager::RegisterGravityBody(UGSGravityBodyComponent* GravityBody)
{
    if (!IsValid(GravityBody))
    {
        return;
    }

    RegisteredGravityBodies.AddUnique(GravityBody);

    if (bHasBegunPlay)
    {
        GravityBody->ApplyGravityState(CurrentState);
    }
}

void AGSGravityManager::UnregisterGravityBody(UGSGravityBodyComponent* GravityBody)
{
    RegisteredGravityBodies.Remove(GravityBody);
}

void AGSGravityManager::PruneInvalidRegistrations()
{
    RegisteredGravityBodies.RemoveAll(
        [](const TObjectPtr<UGSGravityBodyComponent>& Body)
        {
            return !IsValid(Body);
        });
}

FGSGravityState AGSGravityManager::GetGravityState() const
{
    return CurrentState;
}

FVector AGSGravityManager::GetGravityDirection() const
{
    return CurrentState.Direction;
}

TArray<EGSGravityAxis> AGSGravityManager::GetAllowedAxes() const
{
    if (RuleSet && !RuleSet->AllowedAxes.IsEmpty())
    {
        return RuleSet->AllowedAxes;
    }

    return FallbackAllowedAxes;
}

void AGSGravityManager::HandleDebugNextGravity()
{
    RequestNextGravity(this, EGSGravityChangeReason::PlayerInput);
}

void AGSGravityManager::HandleDebugReset()
{
    ResetPuzzleState();
}

void AGSGravityManager::RefreshSurfaceVolumesAfterReset()
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AGSSurfaceModifierVolume> It(GetWorld()); It; ++It)
    {
        It->RefreshOverlappingActors();
    }
}

EGSGravityAxis AGSGravityManager::GetInitialAxis() const
{
    return RuleSet ? RuleSet->InitialAxis : EGSGravityAxis::ZNegative;
}

float AGSGravityManager::GetGravityMagnitude() const
{
    return RuleSet ? RuleSet->GravityMagnitudeCms2 : FallbackGravityMagnitudeCms2;
}

float AGSGravityManager::GetManualCooldown() const
{
    return RuleSet ? RuleSet->ManualChangeCooldown : FallbackManualCooldown;
}

float AGSGravityManager::GetAutoReverseCooldown() const
{
    return RuleSet ? RuleSet->LandingAutoReverseCooldown : FallbackAutoReverseCooldown;
}

bool AGSGravityManager::IsAxisAllowed(const EGSGravityAxis Axis) const
{
    return GetAllowedAxes().Contains(Axis);
}

void AGSGravityManager::InitializeState()
{
    CurrentState.Axis = GetInitialAxis();
    CurrentState.Direction = UGSGravityMathLibrary::AxisToDirection(CurrentState.Axis).GetSafeNormal();
    CurrentState.MagnitudeCms2 = GetGravityMagnitude();
    CurrentState.Revision = 0;
    CurrentState.Reason = EGSGravityChangeReason::Reset;
    CurrentState.InstigatorActor = this;
    CurrentState.WorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    NextManualChangeTime = 0.0f;
    NextAutoReverseTime = 0.0f;
    bTransitionBusy = false;
}

void AGSGravityManager::ApplyCurrentStateToRegisteredBodies()
{
    PruneInvalidRegistrations();

    for (UGSGravityBodyComponent* Body : RegisteredGravityBodies)
    {
        if (IsValid(Body))
        {
            Body->ApplyGravityState(CurrentState);
        }
    }
}

void AGSGravityManager::BroadcastRejected(
    const EGSGravityRequestResult Result,
    const EGSGravityAxis RequestedAxis)
{
    OnGravityRejected.Broadcast(Result, RequestedAxis);
}
