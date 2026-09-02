#pragma once

#include "CoreMinimal.h"
#include "GravityShiftTypes.generated.h"

class AActor;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EGSGravityAxis : uint8
{
    XPositive UMETA(DisplayName = "+X"),
    XNegative UMETA(DisplayName = "-X"),
    YPositive UMETA(DisplayName = "+Y"),
    YNegative UMETA(DisplayName = "-Y"),
    ZPositive UMETA(DisplayName = "+Z"),
    ZNegative UMETA(DisplayName = "-Z")
};

UENUM(BlueprintType)
enum class EGSGravityChangeReason : uint8
{
    PlayerInput,
    LandingAutoReverse,
    Scripted,
    Reset,
    Debug
};

UENUM(BlueprintType)
enum class EGSGravityRequestResult : uint8
{
    Accepted,
    Cooldown,
    TransitionBusy,
    DirectionNotAllowed,
    NoManager,
    InvalidDirection
};

UENUM(BlueprintType)
enum class EGSImpactTier : uint8
{
    None,
    Light,
    Heavy,
    Critical
};

UENUM(BlueprintType)
enum class EGSDestructionMode : uint8
{
    HideAndDisable,
    SwapMesh,
    GeometryCollectionOptional
};

USTRUCT(BlueprintType)
struct GRAVITYSHIFTCORE_API FGSGravityState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    EGSGravityAxis Axis = EGSGravityAxis::ZNegative;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    FVector Direction = FVector(0.0, 0.0, -1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift", meta = (ClampMin = "0.0", Units = "cm/s^2"))
    float MagnitudeCms2 = 980.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    int32 Revision = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    EGSGravityChangeReason Reason = EGSGravityChangeReason::Reset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    TObjectPtr<AActor> InstigatorActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift", meta = (Units = "s"))
    float WorldTimeSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct GRAVITYSHIFTCORE_API FGSSurfaceModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    FName ProfileId = TEXT("Normal");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0"))
    float GravityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0", Units = "Hz"))
    float GravityDragPerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0", Units = "Hz"))
    float TangentialDragPerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0", Units = "cm/s"))
    float TerminalGravitySpeedCms = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0"))
    float ImpactEnergyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "0.0"))
    float BounceMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
    bool bSuppressLandingResponse = false;
};

USTRUCT(BlueprintType)
struct GRAVITYSHIFTCORE_API FGSImpactPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    TObjectPtr<UPrimitiveComponent> SourceComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    TObjectPtr<AActor> HitActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    FVector HitLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    FVector HitNormal = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (Units = "cm/s"))
    float RelativeNormalSpeedCms = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = (Units = "kg"))
    float SourceMassKg = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact", meta = ())
    float EnergyJ = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    EGSImpactTier ImpactTier = EGSImpactTier::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    bool bHasBreakPower = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
    int32 GravityRevision = 0;
};

USTRUCT(BlueprintType)
struct GRAVITYSHIFTCORE_API FGSResetSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset", meta = (Units = "cm/s"))
    FVector LinearVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset", meta = (Units = "deg/s"))
    FVector AngularVelocityDeg = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
    bool bHidden = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
    bool bCollisionEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reset")
    int32 CustomStateInt = 0;
};

USTRUCT(BlueprintType)
struct GRAVITYSHIFTCORE_API FGSGravityBasis
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    FVector Forward = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    FVector Right = FVector::RightVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Shift")
    FVector Up = FVector::UpVector;
};
