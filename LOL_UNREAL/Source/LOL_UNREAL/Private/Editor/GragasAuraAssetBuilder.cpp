// Editor-only authoring for Gragas W and E purple auras.
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
#include "Particles/Location/ParticleModuleLocationPrimitiveSphere.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Size/ParticleModuleSizeMultiplyLife.h"
#include "Particles/Color/ParticleModuleColorOverLife.h"
#include "Distributions/DistributionFloatConstant.h"
#include "Distributions/DistributionFloatUniform.h"
#include "Distributions/DistributionFloatConstantCurve.h"
#include "Distributions/DistributionVectorConstant.h"
#include "Distributions/DistributionVectorUniform.h"
#include "Distributions/DistributionVectorConstantCurve.h"

namespace GragasAuraAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    auto* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

void AddLayer(UParticleSystem* System, UMaterialInterface* Material, FName Name,
    int32 Count, float LifeMin, float LifeMax, FVector Size, FVector Color,
    float Radius, float RadialVelocity, bool bStretched, float StartScale, float EndScale, float SpawnRate = 0.f, bool bTrail = false)
{
    auto* Emitter = NewObject<UParticleSpriteEmitter>(System, NAME_None, RF_Transactional);
    Emitter->SetEmitterName(Name);
    auto* LOD = NewObject<UParticleLODLevel>(Emitter);
    LOD->Level = 0;
    LOD->bEnabled = true;
    auto* Required = Module<UParticleModuleRequired>(System);
    Required->Material = Material;
    Required->bUseLocalSpace = SpawnRate > 0.f && !bTrail;
    Required->bKillOnDeactivate = false;
    Required->EmitterDuration = 1.0f;
    Required->EmitterLoops = SpawnRate > 0.f ? 0 : 1;
    Required->ScreenAlignment = bStretched ? PSA_Velocity : PSA_Rectangle;
    LOD->RequiredModule = Required;

    auto* Spawn = Module<UParticleModuleSpawn>(System);
    auto* Rate = NewObject<UDistributionFloatConstant>(Spawn);
    Rate->Constant = SpawnRate;
    Spawn->Rate.Distribution = Rate;
    FParticleBurst Burst;
    Burst.Count = Count;
    Burst.CountLow = Count;
    Burst.Time = 0.f;
    if (Count > 0) Spawn->BurstList.Add(Burst);
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

    if (Radius > 0.f)
    {
        auto* Sphere = Module<UParticleModuleLocationPrimitiveSphere>(System);
        auto* R = NewObject<UDistributionFloatConstant>(Sphere);
        R->Constant = Radius;
        Sphere->StartRadius.Distribution = R;
        Sphere->SurfaceOnly = true;
        Sphere->Velocity = true;
        auto* V = NewObject<UDistributionFloatConstant>(Sphere);
        V->Constant = RadialVelocity;
        Sphere->VelocityScale.Distribution = V;
        LOD->Modules.Add(Sphere);
    }

    if (Radius > 0.f)
    {
        auto* Velocity = Module<UParticleModuleVelocity>(System);
        auto* Drift = NewObject<UDistributionVectorUniform>(Velocity);
        Drift->Min = bTrail ? FVector(-180, -30, -15) : FVector(-5, -5, 20);
        Drift->Max = bTrail ? FVector(-80, 30, 40) : FVector(5, 5, 45);
        Velocity->bInWorldSpace = false;
        Velocity->StartVelocity.Distribution = Drift;
        LOD->Modules.Add(Velocity);
    }

    auto* Fade = Module<UParticleModuleColorOverLife>(System);
    auto* Tint = NewObject<UDistributionVectorConstantCurve>(Fade);
    Tint->ConstantCurve.AddPoint(0.f, Color);
    Tint->ConstantCurve.AddPoint(1.f, Color * 0.7f);
    Fade->ColorOverLife.Distribution = Tint;
    auto* Alpha = NewObject<UDistributionFloatConstantCurve>(Fade);
    Alpha->ConstantCurve.AddPoint(0.f, SpawnRate > 0.f ? 0.f : 0.85f);
    Alpha->ConstantCurve.AddPoint(0.15f, 1.f);
    Alpha->ConstantCurve.AddPoint(0.5f, 0.4f);
    Alpha->ConstantCurve.AddPoint(1.f, 0.f);
    Fade->AlphaOverLife.Distribution = Alpha;
    LOD->Modules.Add(Fade);

    auto* Scale = Module<UParticleModuleSizeMultiplyLife>(System);
    auto* Curve = NewObject<UDistributionVectorConstantCurve>(Scale);
    Curve->ConstantCurve.AddPoint(0.f, FVector(StartScale));
    Curve->ConstantCurve.AddPoint(1.f, FVector(EndScale));
    Scale->LifeMultiplier.Distribution = Curve;
    Scale->MultiplyX = Scale->MultiplyY = Scale->MultiplyZ = true;
    LOD->Modules.Add(Scale);
    Emitter->LODLevels.Add(LOD);
    System->Emitters.Add(Emitter);
}

void Build(bool bDash)
{
    auto* Ring = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitRing.M_HitRing"));
    auto* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitFlash.M_HitFlash"));
    auto* Spark = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitSpark.M_HitSpark"));
    if (!Ring || !Glow || !Spark) return;
    auto* Wisp = bDash ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/Gragas/M_GragasDashWisp.M_GragasDashWisp")) : nullptr;
    if (bDash && !Wisp) return;
    const FString Name = bDash ? TEXT("P_GragasEDashAura") : TEXT("P_GragasWAura");
    const FString Path = TEXT("/Game/VFX/Gragas/") + Name;
    UPackage* Package = CreatePackage(*Path);
    Package->FullyLoad();
    auto* System = FindObject<UParticleSystem>(Package, *Name);
    if (!System) System = NewObject<UParticleSystem>(Package, *Name, RF_Public | RF_Standalone);
    System->Modify();
    System->Emitters.Reset();
    System->LODDistances = {0.f};
    System->LODSettings.SetNum(1);
    System->WarmupTime = bDash ? 0.f : 0.35f;
    System->bUseFixedRelativeBoundingBox = false;
    if (bDash)
    {
        // Short-lived world-space contours and streaks shed behind the moving body.
        AddLayer(System, Wisp, TEXT("01_DashWake"), 5, .18f, .26f,
            FVector(65, 190, 65), FVector(.5f, .08f, .9f), 60, .15f, true, 1.f, .5f, 36.f, true);
        AddLayer(System, Glow, TEXT("02_VioletWisps"), 12, .18f, .32f,
            FVector(30), FVector(.45f, .025f, .8f), 65, .3f, false, 1.f, .2f, 100.f, true);
        AddLayer(System, Spark, TEXT("03_LavenderSpeedStreaks"), 12, .16f, .28f,
            FVector(7, 60, 7), FVector(.7f, .2f, 1.f), 70, .2f, true, 1.f, .1f, 100.f, true);
    }
    else
    {
    // Broad, soft purple contours breathe outward around the torso.
    AddLayer(System, Ring, TEXT("01_VioletAura"), 1, .9f, .9f,
        FVector(230, 210, 230), FVector(.48f, .015f, .85f), 0, 0, false, .75f, 1.15f, 2.2f);
    AddLayer(System, Ring, TEXT("02_LavenderInnerGlow"), 1, .7f, .7f,
        FVector(160, 220, 160), FVector(.25f, .035f, .55f), 0, 0, false, .85f, 1.08f, 2.f);
    AddLayer(System, Glow, TEXT("03_SoftVioletMotes"), 0, .65f, 1.05f,
        FVector(16), FVector(.38f, .015f, .7f), 80, .15f, false, .3f, 1.f, 34.f);
    AddLayer(System, Spark, TEXT("04_RisingMagic"), 0, .55f, .85f,
        FVector(5, 20, 5), FVector(.65f, .12f, 1.f), 65, .2f, false, .4f, .1f, 20.f);
    }
    System->UpdateAllModuleLists();
    System->BuildEmitters();
    System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
    const bool Saved = UPackage::SavePackage(Package, System, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("Gragas aura %s saved: %d"), *Name, Saved);
}
static void BuildW() { Build(false); }
static void BuildE() { Build(true); }
static FAutoConsoleCommand BuildCommand(TEXT("LOL.Gragas.BuildWAura"), TEXT("Build the purple Gragas W aura asset."), FConsoleCommandDelegate::CreateStatic(&BuildW));
static FAutoConsoleCommand BuildECommand(TEXT("LOL.Gragas.BuildEAura"), TEXT("Build the Gragas E dash trail asset."), FConsoleCommandDelegate::CreateStatic(&BuildE));
}
#endif
