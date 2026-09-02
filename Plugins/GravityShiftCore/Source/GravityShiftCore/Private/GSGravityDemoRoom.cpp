#include "GSGravityDemoRoom.h"

#include "Components/ChildActorComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GSGravityBlock.h"
#include "GSGravityManager.h"
#include "GSSurfaceModifierVolume.h"

AGSGravityDemoRoom::AGSGravityDemoRoom()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    auto CreateWall = [this](
        const FName ComponentName,
        const FVector& RelativeLocation,
        const FVector& RelativeScale)
    {
        UStaticMeshComponent* Wall =
            CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
        Wall->SetupAttachment(SceneRoot);
        Wall->SetMobility(EComponentMobility::Static);
        Wall->SetCollisionProfileName(TEXT("BlockAll"));
        Wall->SetRelativeLocation(RelativeLocation);
        Wall->SetRelativeScale3D(RelativeScale);
        if (CubeMesh.Succeeded())
        {
            Wall->SetStaticMesh(CubeMesh.Object);
        }
        return Wall;
    };

    Floor = CreateWall(
        TEXT("Floor"),
        FVector(0.0, 0.0, -350.0),
        FVector(12.0, 12.0, 0.40));

    Ceiling = CreateWall(
        TEXT("Ceiling"),
        FVector(0.0, 0.0, 350.0),
        FVector(12.0, 12.0, 0.40));

    WallPositiveX = CreateWall(
        TEXT("WallPositiveX"),
        FVector(600.0, 0.0, 0.0),
        FVector(0.40, 12.0, 7.0));

    WallNegativeX = CreateWall(
        TEXT("WallNegativeX"),
        FVector(-600.0, 0.0, 0.0),
        FVector(0.40, 12.0, 7.0));

    WallPositiveY = CreateWall(
        TEXT("WallPositiveY"),
        FVector(0.0, 600.0, 0.0),
        FVector(12.0, 0.40, 7.0));

    WallNegativeY = CreateWall(
        TEXT("WallNegativeY"),
        FVector(0.0, -600.0, 0.0),
        FVector(12.0, 0.40, 7.0));

    GravityManagerActor =
        CreateDefaultSubobject<UChildActorComponent>(TEXT("GravityManager"));
    GravityManagerActor->SetupAttachment(SceneRoot);
    GravityManagerActor->SetChildActorClass(AGSGravityManager::StaticClass());

    GravityBlockActor =
        CreateDefaultSubobject<UChildActorComponent>(TEXT("GravityBlock"));
    GravityBlockActor->SetupAttachment(SceneRoot);
    GravityBlockActor->SetChildActorClass(AGSGravityBlock::StaticClass());
    GravityBlockActor->SetRelativeLocation(FVector(0.0, 0.0, 0.0));

    SurfaceModifierActor =
        CreateDefaultSubobject<UChildActorComponent>(TEXT("SurfaceModifier"));
    SurfaceModifierActor->SetupAttachment(SceneRoot);
    SurfaceModifierActor->SetChildActorClass(AGSSurfaceModifierVolume::StaticClass());
    SurfaceModifierActor->SetRelativeLocation(FVector(-560.0, 0.0, 0.0));

    InstructionsText =
        CreateDefaultSubobject<UTextRenderComponent>(TEXT("Instructions"));
    InstructionsText->SetupAttachment(SceneRoot);
    InstructionsText->SetRelativeLocation(FVector(0.0, -300.0, 250.0));
    InstructionsText->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    InstructionsText->SetHorizontalAlignment(EHTA_Center);
    InstructionsText->SetWorldSize(38.0f);
    InstructionsText->SetText(FText::FromString(TEXT("Press G: next gravity axis")));
}
