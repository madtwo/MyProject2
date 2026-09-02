#include "GSSurfaceModifierVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GSGravityBodyComponent.h"
#include "GravityShiftProfiles.h"

AGSSurfaceModifierVolume::AGSSurfaceModifierVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    SurfaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SM_Surface"));
    SurfaceMesh->SetupAttachment(SceneRoot);
    SurfaceMesh->SetMobility(EComponentMobility::Static);
    SurfaceMesh->SetCollisionProfileName(TEXT("BlockAll"));
    SurfaceMesh->SetRelativeScale3D(FVector(0.20, 4.0, 4.0));

    InfluenceVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Box_Influence"));
    InfluenceVolume->SetupAttachment(SceneRoot);
    InfluenceVolume->SetMobility(EComponentMobility::Static);
    InfluenceVolume->SetBoxExtent(FVector(30.0, 210.0, 210.0));
    InfluenceVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InfluenceVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InfluenceVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InfluenceVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    InfluenceVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    InfluenceVolume->SetGenerateOverlapEvents(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        SurfaceMesh->SetStaticMesh(CubeMesh.Object);
    }

    InlineProfileA.ProfileId = TEXT("SlowSurface");
    InlineProfileA.Priority = 10;
    InlineProfileA.GravityMultiplier = 0.35f;
    InlineProfileA.GravityDragPerSecond = 1.5f;
    InlineProfileA.TangentialDragPerSecond = 0.8f;
    InlineProfileA.TerminalGravitySpeedCms = 1200.0f;
    InlineProfileA.ImpactEnergyMultiplier = 0.8f;

    InlineProfileB.ProfileId = TEXT("FastSurface");
    InlineProfileB.Priority = 10;
    InlineProfileB.GravityMultiplier = 1.35f;
    InlineProfileB.GravityDragPerSecond = 0.0f;
    InlineProfileB.TangentialDragPerSecond = 0.0f;
    InlineProfileB.TerminalGravitySpeedCms = 4200.0f;
    InlineProfileB.ImpactEnergyMultiplier = 1.3f;
}

void AGSSurfaceModifierVolume::BeginPlay()
{
    Super::BeginPlay();

    InfluenceVolume->OnComponentBeginOverlap.AddUniqueDynamic(
        this,
        &AGSSurfaceModifierVolume::HandleBeginOverlap);

    InfluenceVolume->OnComponentEndOverlap.AddUniqueDynamic(
        this,
        &AGSSurfaceModifierVolume::HandleEndOverlap);
}

bool AGSSurfaceModifierVolume::ToggleProfile()
{
    if (!bCanBeToggled || UsesRemaining == 0)
    {
        return false;
    }

    if (UsesRemaining > 0)
    {
        --UsesRemaining;
    }

    SetUseProfileB(!bUseProfileB);
    return true;
}

void AGSSurfaceModifierVolume::SetUseProfileB(const bool bNewUseProfileB)
{
    if (bUseProfileB == bNewUseProfileB)
    {
        return;
    }

    bUseProfileB = bNewUseProfileB;
    RefreshOverlappingActors();
    OnSurfaceProfileChanged.Broadcast(bUseProfileB, GetActiveModifier());
}

void AGSSurfaceModifierVolume::RefreshOverlappingActors()
{
    for (AActor* Actor : TrackedOverlappingActors)
    {
        if (IsValid(Actor))
        {
            RemoveModifierFromActor(Actor);
        }
    }
    TrackedOverlappingActors.Reset();

    TArray<AActor*> CurrentOverlaps;
    InfluenceVolume->GetOverlappingActors(CurrentOverlaps);
    for (AActor* Actor : CurrentOverlaps)
    {
        if (IsValid(Actor) && Actor != this)
        {
            TrackedOverlappingActors.Add(Actor);
            ApplyModifierToActor(Actor);
        }
    }
}

FGSSurfaceModifier AGSSurfaceModifierVolume::GetActiveModifier() const
{
    if (bUseProfileB)
    {
        return ProfileB ? ProfileB->Modifier : InlineProfileB;
    }

    return ProfileA ? ProfileA->Modifier : InlineProfileA;
}

void AGSSurfaceModifierVolume::HandleBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!IsValid(OtherActor) || OtherActor == this)
    {
        return;
    }

    TrackedOverlappingActors.Add(OtherActor);
    ApplyModifierToActor(OtherActor);
}

void AGSSurfaceModifierVolume::HandleEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex)
{
    if (!OtherActor)
    {
        return;
    }

    RemoveModifierFromActor(OtherActor);
    TrackedOverlappingActors.Remove(OtherActor);
}

void AGSSurfaceModifierVolume::ApplyModifierToActor(AActor* TargetActor)
{
    if (!IsValid(TargetActor))
    {
        return;
    }

    TArray<UGSGravityBodyComponent*> GravityBodies;
    TargetActor->GetComponents<UGSGravityBodyComponent>(GravityBodies);

    const FGSSurfaceModifier Modifier = GetActiveModifier();
    for (UGSGravityBodyComponent* Body : GravityBodies)
    {
        if (IsValid(Body))
        {
            Body->PushSurfaceModifier(this, Modifier);
        }
    }
}

void AGSSurfaceModifierVolume::RemoveModifierFromActor(AActor* TargetActor)
{
    if (!IsValid(TargetActor))
    {
        return;
    }

    TArray<UGSGravityBodyComponent*> GravityBodies;
    TargetActor->GetComponents<UGSGravityBodyComponent>(GravityBodies);

    for (UGSGravityBodyComponent* Body : GravityBodies)
    {
        if (IsValid(Body))
        {
            Body->PopSurfaceModifier(this);
        }
    }
}

void AGSSurfaceModifierVolume::PruneTrackedActors()
{
    for (auto It = TrackedOverlappingActors.CreateIterator(); It; ++It)
    {
        if (!IsValid(*It))
        {
            It.RemoveCurrent();
        }
    }
}
