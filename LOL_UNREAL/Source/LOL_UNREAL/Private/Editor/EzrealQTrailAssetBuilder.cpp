// Editor-only authoring command. The saved asset is used normally by packaged builds.
#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/Lifetime/ParticleModuleLifetime.h"
#include "Particles/Location/ParticleModuleLocation.h"
#include "Particles/Velocity/ParticleModuleVelocity.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Size/ParticleModuleSizeMultiplyLife.h"
#include "Particles/Color/ParticleModuleColorOverLife.h"
#include "Distributions/DistributionFloatConstant.h"
#include "Distributions/DistributionFloatUniform.h"
#include "Distributions/DistributionFloatConstantCurve.h"
#include "Distributions/DistributionVectorConstant.h"
#include "Distributions/DistributionVectorUniform.h"
#include "Distributions/DistributionVectorConstantCurve.h"

namespace EzrealQTrailAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    T* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

UDistributionVectorUniform* VectorRange(UObject* Outer, FVector Min, FVector Max)
{
    auto* D = NewObject<UDistributionVectorUniform>(Outer);
    D->Min = Min; D->Max = Max;
    return D;
}

void AddEmitter(UParticleSystem* System, UMaterialInterface* Material, FName Name, float Rate, FVector Color, float Size)
{
    auto* Emitter = NewObject<UParticleSpriteEmitter>(System, NAME_None, RF_Transactional);
    Emitter->SetEmitterName(Name);
    auto* LOD = NewObject<UParticleLODLevel>(Emitter);
    LOD->Level = 0; LOD->bEnabled = true;
    auto* Required = Module<UParticleModuleRequired>(System);
    Required->Material = Material;
    Required->bUseLocalSpace = false;
    Required->bKillOnDeactivate = false;
    Required->bKillOnCompleted = false;
    Required->EmitterDuration = 1.0f;
    Required->EmitterLoops = 0;
    Required->ScreenAlignment = PSA_Square;
    LOD->RequiredModule = Required;

    auto* Spawn = Module<UParticleModuleSpawn>(System);
    auto* SpawnRate = NewObject<UDistributionFloatConstant>(Spawn);
    SpawnRate->Constant = Rate;
    Spawn->Rate.Distribution = SpawnRate;
    LOD->SpawnModule = Spawn;

    auto* Lifetime = Module<UParticleModuleLifetime>(System);
    auto* LifeRange = NewObject<UDistributionFloatUniform>(Lifetime);
    LifeRange->Min = 0.18f; LifeRange->Max = 0.42f;
    Lifetime->Lifetime.Distribution = LifeRange;
    LOD->Modules.Add(Lifetime);

    auto* Location = Module<UParticleModuleLocation>(System);
    Location->StartLocation.Distribution = VectorRange(Location, FVector(-28, -7, -7), FVector(-12, 7, 7));
    LOD->Modules.Add(Location);

    auto* Velocity = Module<UParticleModuleVelocity>(System);
    Velocity->bInWorldSpace = false;
    Velocity->StartVelocity.Distribution = VectorRange(Velocity, FVector(-100, -75, -60), FVector(-25, 75, 60));
    LOD->Modules.Add(Velocity);

    auto* InitialSize = Module<UParticleModuleSize>(System);
    InitialSize->StartSize.Distribution = VectorRange(InitialSize, FVector(Size * 0.5f), FVector(Size));
    LOD->Modules.Add(InitialSize);

    auto* Fade = Module<UParticleModuleColorOverLife>(System);
    auto* Tint = NewObject<UDistributionVectorConstant>(Fade);
    Tint->Constant = Color;
    Fade->ColorOverLife.Distribution = Tint;
    auto* Alpha = NewObject<UDistributionFloatConstantCurve>(Fade);
    Alpha->ConstantCurve.AddPoint(0.0f, 0.0f);
    Alpha->ConstantCurve.AddPoint(0.08f, 1.0f);
    Alpha->ConstantCurve.AddPoint(0.45f, 0.8f);
    Alpha->ConstantCurve.AddPoint(1.0f, 0.0f);
    Fade->AlphaOverLife.Distribution = Alpha;
    LOD->Modules.Add(Fade);

    auto* Shrink = Module<UParticleModuleSizeMultiplyLife>(System);
    auto* SizeCurve = NewObject<UDistributionVectorConstantCurve>(Shrink);
    SizeCurve->ConstantCurve.AddPoint(0.0f, FVector(1.0f));
    SizeCurve->ConstantCurve.AddPoint(1.0f, FVector(0.15f));
    Shrink->LifeMultiplier.Distribution = SizeCurve;
    Shrink->MultiplyX = Shrink->MultiplyY = Shrink->MultiplyZ = true;
    LOD->Modules.Add(Shrink);

    Emitter->LODLevels.Add(LOD);
    System->Emitters.Add(Emitter);
}

void Build()
{
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Level/ezreal/FX/M_EzrealQTrail.M_EzrealQTrail"));
    if (!Material)
    {
        UE_LOG(LogTemp, Error, TEXT("Create M_EzrealQTrail with Tools/create_ezreal_q_material.py first."));
        return;
    }
    const FString Path = TEXT("/Game/Level/ezreal/FX/P_EzrealQTrail");
    UPackage* Package = CreatePackage(*Path);
    Package->FullyLoad();
    UParticleSystem* System = FindObject<UParticleSystem>(Package, TEXT("P_EzrealQTrail"));
    if (!System) System = NewObject<UParticleSystem>(Package, TEXT("P_EzrealQTrail"), RF_Public | RF_Standalone);
    System->Modify();
    System->Emitters.Reset();
    System->LODDistances = {0.0f};
    System->LODSettings.SetNum(1);
    System->WarmupTime = 0.0f;
    System->bUseFixedRelativeBoundingBox = false;
    AddEmitter(System, Material, TEXT("ArcaneGold"), 150.0f, FVector(1.0f, 0.58f, 0.12f), 10.0f);
    AddEmitter(System, Material, TEXT("CyanSparks"), 55.0f, FVector(0.12f, 0.7f, 1.0f), 6.0f);
    System->UpdateAllModuleLists();
    System->BuildEmitters();
    System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Package, System, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("Ezreal Q trail asset saved: %d"), bSaved);
}
static FAutoConsoleCommand BuildCommand(TEXT("LOL.Ezreal.BuildQTrail"), TEXT("Rebuild the authored Q trail asset (editor only)."), FConsoleCommandDelegate::CreateStatic(&Build));
}
#endif
