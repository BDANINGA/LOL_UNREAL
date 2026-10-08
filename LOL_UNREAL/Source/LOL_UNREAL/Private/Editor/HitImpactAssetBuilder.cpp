// Asset-authoring command only. No gameplay hooks or runtime spawning.
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

namespace HitImpactAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    auto* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

void AddBurst(UParticleSystem* System, UMaterialInterface* Material, FName Name,
    int32 Count, float LifeMin, float LifeMax, FVector Size, FVector Color,
    float Speed, bool bStretched, bool bExpanding)
{
    auto* Emitter = NewObject<UParticleSpriteEmitter>(System, NAME_None, RF_Transactional);
    Emitter->SetEmitterName(Name);
    auto* LOD = NewObject<UParticleLODLevel>(Emitter);
    LOD->Level = 0;
    LOD->bEnabled = true;
    auto* Required = Module<UParticleModuleRequired>(System);
    Required->Material = Material;
    Required->bUseLocalSpace = false;
    Required->bKillOnDeactivate = false;
    Required->EmitterDuration = 0.5f;
    Required->EmitterLoops = 1;
    Required->ScreenAlignment = bStretched ? PSA_Velocity : PSA_Square;
    LOD->RequiredModule = Required;

    auto* Spawn = Module<UParticleModuleSpawn>(System);
    auto* Rate = NewObject<UDistributionFloatConstant>(Spawn);
    Rate->Constant = 0.f;
    Spawn->Rate.Distribution = Rate;
    FParticleBurst Burst;
    Burst.Count = Count;
    Burst.CountLow = Count;
    Burst.Time = 0.f;
    Spawn->BurstList.Add(Burst);
    LOD->SpawnModule = Spawn;

    auto* Lifetime = Module<UParticleModuleLifetime>(System);
    auto* Life = NewObject<UDistributionFloatUniform>(Lifetime);
    Life->Min = LifeMin;
    Life->Max = LifeMax;
    Lifetime->Lifetime.Distribution = Life;
    LOD->Modules.Add(Lifetime);

    auto* InitialSize = Module<UParticleModuleSize>(System);
    auto* Sizes = NewObject<UDistributionVectorUniform>(InitialSize);
    Sizes->Min = Size * 0.75f;
    Sizes->Max = Size;
    InitialSize->StartSize.Distribution = Sizes;
    LOD->Modules.Add(InitialSize);

    if (Speed > 0.f)
    {
        auto* Velocity = Module<UParticleModuleVelocity>(System);
        auto* Range = NewObject<UDistributionVectorUniform>(Velocity);
        Range->Min = FVector(-Speed, -Speed, -Speed * 0.6f);
        Range->Max = FVector(Speed, Speed, Speed * 0.8f);
        Velocity->StartVelocity.Distribution = Range;
        LOD->Modules.Add(Velocity);
    }

    auto* Fade = Module<UParticleModuleColorOverLife>(System);
    auto* Tint = NewObject<UDistributionVectorConstantCurve>(Fade);
    Tint->ConstantCurve.AddPoint(0.f, Color);
    Tint->ConstantCurve.AddPoint(1.f, Color * FVector(1.f, 0.55f, 0.3f));
    Fade->ColorOverLife.Distribution = Tint;
    auto* Alpha = NewObject<UDistributionFloatConstantCurve>(Fade);
    Alpha->ConstantCurve.AddPoint(0.f, bExpanding ? 0.6f : 1.f);
    Alpha->ConstantCurve.AddPoint(0.15f, bExpanding ? 0.65f : 1.f);
    Alpha->ConstantCurve.AddPoint(0.5f, 0.4f);
    Alpha->ConstantCurve.AddPoint(1.f, 0.f);
    Fade->AlphaOverLife.Distribution = Alpha;
    LOD->Modules.Add(Fade);

    auto* Scale = Module<UParticleModuleSizeMultiplyLife>(System);
    auto* Curve = NewObject<UDistributionVectorConstantCurve>(Scale);
    Curve->ConstantCurve.AddPoint(0.f, FVector(bExpanding ? 0.25f : 1.f));
    Curve->ConstantCurve.AddPoint(1.f, FVector(bExpanding ? 1.5f : 0.1f));
    Scale->LifeMultiplier.Distribution = Curve;
    Scale->MultiplyX = Scale->MultiplyY = Scale->MultiplyZ = true;
    LOD->Modules.Add(Scale);
    Emitter->LODLevels.Add(LOD);
    System->Emitters.Add(Emitter);
}

void Build()
{
    auto* Flash = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitFlash.M_HitFlash"));
    auto* Spark = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitSpark.M_HitSpark"));
    auto* Ring = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitRing.M_HitRing"));
    if (!Flash || !Spark || !Ring)
    {
        UE_LOG(LogTemp, Error, TEXT("Run Tools/create_hit_impact_materials.py first."));
        return;
    }
    const FString Path = TEXT("/Game/VFX/HitImpact/P_HitImpact");
    UPackage* Package = CreatePackage(*Path);
    Package->FullyLoad();
    auto* System = FindObject<UParticleSystem>(Package, TEXT("P_HitImpact"));
    if (!System) System = NewObject<UParticleSystem>(Package, TEXT("P_HitImpact"), RF_Public | RF_Standalone);
    System->Modify();
    System->Emitters.Reset();
    System->LODDistances = {0.f};
    System->LODSettings.SetNum(1);
    System->WarmupTime = 0.f;
    System->bUseFixedRelativeBoundingBox = false;
    AddBurst(System, Flash, TEXT("01_ImpactFlash"), 1, 0.09f, 0.09f, FVector(95.f), FVector(1.f, 0.9f, 0.65f), 0.f, false, false);
    AddBurst(System, Ring, TEXT("02_ShockRing"), 1, 0.2f, 0.2f, FVector(130.f), FVector(1.f, 0.52f, 0.12f), 0.f, false, true);
    AddBurst(System, Spark, TEXT("03_RadialShards"), 16, 0.16f, 0.3f, FVector(5.f, 32.f, 5.f), FVector(1.f, 0.65f, 0.22f), 420.f, true, false);
    AddBurst(System, Flash, TEXT("04_EmberMotes"), 10, 0.22f, 0.42f, FVector(7.f), FVector(1.f, 0.4f, 0.08f), 180.f, false, false);
    System->UpdateAllModuleLists();
    System->BuildEmitters();
    System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Package, System, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("Standalone hit impact asset saved: %d"), bSaved);
}
static FAutoConsoleCommand BuildCommand(TEXT("LOL.VFX.BuildHitImpact"), TEXT("Author the standalone hit effect; does not connect it to gameplay."), FConsoleCommandDelegate::CreateStatic(&Build));
}
#endif
