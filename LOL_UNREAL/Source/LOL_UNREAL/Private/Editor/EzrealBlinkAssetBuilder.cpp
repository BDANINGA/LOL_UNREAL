// Editor-only authoring for Ezreal E departure/arrival bursts.
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

namespace EzrealBlinkAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    auto* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

void AddBurst(UParticleSystem* System, UMaterialInterface* Material, FName Name,
    int32 Count, float LifeMin, float LifeMax, FVector Size, FVector Color,
    float Radius, float RadialVelocity, bool bStretched, float StartScale, float EndScale)
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
    Required->ScreenAlignment = bStretched ? PSA_Velocity : PSA_Rectangle;
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

    auto* Fade = Module<UParticleModuleColorOverLife>(System);
    auto* Tint = NewObject<UDistributionVectorConstantCurve>(Fade);
    Tint->ConstantCurve.AddPoint(0.f, Color);
    Tint->ConstantCurve.AddPoint(1.f, Color * 0.7f);
    Fade->ColorOverLife.Distribution = Tint;
    auto* Alpha = NewObject<UDistributionFloatConstantCurve>(Fade);
    Alpha->ConstantCurve.AddPoint(0.f, 0.85f);
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

void Build(bool bArrival)
{
    auto* Flash = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitFlash.M_HitFlash"));
    auto* Spark = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitSpark.M_HitSpark"));
    auto* Ring = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitRing.M_HitRing"));
    if (!Flash || !Spark || !Ring)
    {
        UE_LOG(LogTemp, Error, TEXT("Run Tools/create_hit_impact_materials.py first."));
        return;
    }
    const FString Name = bArrival ? TEXT("P_EzrealEArrival") : TEXT("P_EzrealEDeparture");
    const FString Path = TEXT("/Game/Level/ezreal/FX/E/") + Name;
    UPackage* Package = CreatePackage(*Path);
    Package->FullyLoad();
    auto* System = FindObject<UParticleSystem>(Package, *Name);
    if (!System) System = NewObject<UParticleSystem>(Package, *Name, RF_Public | RF_Standalone);
    System->Modify();
    System->Emitters.Reset();
    System->LODDistances = {0.f};
    System->LODSettings.SetNum(1);
    System->WarmupTime = 0.f;
    System->bUseFixedRelativeBoundingBox = false;
    const FVector Gold(1.f, 0.65f, 0.16f);
    const FVector Cyan(0.08f, 0.7f, 1.f);
    AddBurst(System, Ring, TEXT("01_ArcaneRing"), 1, 0.32f, 0.32f,
        FVector(bArrival ? 200.f : 180.f), Gold, 0.f, 0.f, false,
        bArrival ? 0.2f : 1.1f, bArrival ? 1.25f : 0.1f);
    AddBurst(System, Flash, TEXT("02_BlinkFlash"), 1, 0.12f, 0.12f,
        FVector(115.f), FVector(1.f, 0.92f, 0.7f), 0.f, 0.f, false, 1.f, 0.15f);
    AddBurst(System, Flash, TEXT("03_VerticalFlare"), 1, 0.22f, 0.22f,
        FVector(45.f, 220.f, 45.f), Gold, 0.f, 0.f, false, 1.f, 0.1f);
    AddBurst(System, Spark, TEXT("04_GoldShards"), bArrival ? 38 : 28, 0.22f, 0.36f,
        FVector(5.f, 28.f, 5.f), Gold, bArrival ? 12.f : 65.f,
        bArrival ? 22.f : -2.8f, true, 1.f, 0.1f);
    AddBurst(System, Flash, TEXT("05_ArcaneMotes"), bArrival ? 30 : 22, 0.3f, 0.6f,
        FVector(9.f), Cyan, bArrival ? 20.f : 48.f,
        bArrival ? 5.f : -1.5f, false, 1.f, 0.1f);
    System->UpdateAllModuleLists();
    System->BuildEmitters();
    System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Package, System, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("Ezreal E asset %s saved: %d"), *Name, bSaved);
}
static void BuildBoth() { Build(false); Build(true); }
static FAutoConsoleCommand BuildCommand(TEXT("LOL.Ezreal.BuildBlink"), TEXT("Rebuild Ezreal E departure and arrival assets."), FConsoleCommandDelegate::CreateStatic(&BuildBoth));
}
#endif
