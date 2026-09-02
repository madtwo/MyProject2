#include "GravityShiftCore.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGravityShift);

void FGravityShiftCoreModule::StartupModule()
{
    UE_LOG(LogGravityShift, Log, TEXT("GravityShiftCore module started."));
}

void FGravityShiftCoreModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FGravityShiftCoreModule, GravityShiftCore)
