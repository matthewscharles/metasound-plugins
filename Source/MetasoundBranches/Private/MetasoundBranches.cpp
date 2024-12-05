#include "MetasoundBranches/Public/MetasoundBranches.h"
#include "MetasoundFrontendRegistries.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FMetasoundBranchesModule"

void FMetasoundBranchesModule::StartupModule()
{
    // Initialization
    UE_LOG(LogTemp, Log, TEXT("MetasoundBranches module started..."));
}

void FMetasoundBranchesModule::ShutdownModule()
{
    // Cleanup
    UE_LOG(LogTemp, Log, TEXT("MetasoundBranches module shutting down..."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetasoundBranchesModule, MetasoundBranches);