#include "GSGravityBlock.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "GSBreakableComponent.h"
#include "GSGravityBodyComponent.h"
#include "GSResettableComponent.h"
#include "GravityShiftProfiles.h"
#include "PhysicsEngine/BodyInstance.h"

AGSGravityBlock::AGSGravityBlock()
{
    PrimaryActorTick.bCanEverTick = false;

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SM_Body"));
    SetRootComponent(BodyMesh);
    BodyMesh->SetMobility(EComponentMobility::Movable);
    BodyMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    BodyMesh->SetNotifyRigidBodyCollision(true);
    BodyMesh->SetEnableGravity(false);
    BodyMesh->SetSimulatePhysics(true);
    BodyMesh->ComponentTags.AddUnique(TEXT("GravityBody"));
    BodyMesh->ComponentTags.AddUnique(TEXT("BreakableBody"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMesh.Object);
    }

    GravityBody = CreateDefaultSubobject<UGSGravityBodyComponent>(TEXT("BPC_GravityBody"));
    Breakable = CreateDefaultSubobject<UGSBreakableComponent>(TEXT("BPC_Breakable"));
    Resettable = CreateDefaultSubobject<UGSResettableComponent>(TEXT("BPC_Resettable"));
}

void AGSGravityBlock::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyBlockProfileInternal(true);
}

void AGSGravityBlock::BeginPlay()
{
    Super::BeginPlay();
    ApplyBlockProfileInternal(true);
}

void AGSGravityBlock::ApplyBlockProfile()
{
    ApplyBlockProfileInternal(HasActorBegunPlay());
}

void AGSGravityBlock::ResetBlock()
{
    if (IsValid(Resettable))
    {
        Resettable->RestoreResetSnapshot();
    }

    if (IsValid(Breakable))
    {
        Breakable->RestoreBreakState();
    }
}

void AGSGravityBlock::ApplyBlockProfileInternal(const bool bApplyRuntimePhysics)
{
    const bool bConfiguredSimulatePhysics =
        BlockProfile ? BlockProfile->bSimulatePhysics : bSimulatePhysics;
    const bool bConfiguredGravityAffected =
        BlockProfile ? BlockProfile->bGravityAffected : bGravityAffected;
    const bool bConfiguredBreakable =
        BlockProfile ? BlockProfile->bBreakable : bBreakable;
    const float ConfiguredMassKg =
        BlockProfile ? BlockProfile->MassOverrideKg : MassOverrideKg;
    const float ConfiguredGravityScale =
        BlockProfile ? BlockProfile->GravityScale : GravityScale;
    const bool bConfiguredUseCCD =
        BlockProfile ? BlockProfile->bUseCCD : bUseCCD;

    if (IsValid(BodyMesh))
    {
        BodyMesh->SetMobility(EComponentMobility::Movable);
        BodyMesh->SetMassOverrideInKg(
            NAME_None,
            FMath::Max(0.001f, ConfiguredMassKg),
            true);
        BodyMesh->SetNotifyRigidBodyCollision(true);
        BodyMesh->BodyInstance.bUseCCD = bConfiguredUseCCD;
        BodyMesh->SetEnableGravity(false);

        if (bApplyRuntimePhysics)
        {
            BodyMesh->SetSimulatePhysics(bConfiguredSimulatePhysics);
            if (bConfiguredSimulatePhysics)
            {
                BodyMesh->WakeAllRigidBodies();
            }
        }
    }

    if (IsValid(GravityBody))
    {
        GravityBody->GravityScale = FMath::Max(0.0f, ConfiguredGravityScale);
        GravityBody->bUseCCD = bConfiguredUseCCD;
        GravityBody->SetRuntimeEnabled(
            bConfiguredGravityAffected && bConfiguredSimulatePhysics);
    }

    if (IsValid(Breakable))
    {
        Breakable->BreakProfile =
            BlockProfile ? BlockProfile->BreakProfile : Breakable->BreakProfile;
        Breakable->SetRuntimeEnabled(bConfiguredBreakable);
    }
}
