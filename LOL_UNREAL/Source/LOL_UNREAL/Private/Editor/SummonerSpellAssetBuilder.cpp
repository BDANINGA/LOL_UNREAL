// Editor-only authoring for summoner Flash bursts and a looping Exhaust aura.
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

namespace SummonerSpellAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    auto* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

void AddBurst(UParticleSystem* System, UMaterialInterface* Material, FName Name,
    int32 Count, float LifeMin, float LifeMax, FVector Size, FVector Color,
    float Radius, float RadialVelocity, bool bStretched, float StartScale, float EndScale, float SpawnRate = 0.f)
{
    auto* Emitter = NewObject<UParticleSpriteEmitter>(System, NAME_None, RF_Transactional);
    Emitter->SetEmitterName(Name);
    auto* LOD = NewObject<UParticleLODLevel>(Emitter);
    LOD->Level = 0;
    LOD->bEnabled = true;
    auto* Required = Module<UParticleModuleRequired>(System);
    Required->Material = Material;
    Required->bUseLocalSpace = SpawnRate > 0.f;
    Required->bKillOnDeactivate = false;
    Required->EmitterDuration = 0.5f;
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

void Build(int32 Kind)
{
    auto* Flash = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitFlash.M_HitFlash"));
    auto* Spark = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitSpark.M_HitSpark"));
    auto* Ring = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/HitImpact/M_HitRing.M_HitRing"));
    if (!Flash || !Spark || !Ring) return;
    auto* Star = Kind != 2 ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/SummonerSpells/M_FlashStar.M_FlashStar")) : nullptr;
    auto* Slash = Kind != 2 ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/SummonerSpells/M_FlashSlash.M_FlashSlash")) : nullptr;
    auto* Chip = Kind != 2 ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/SummonerSpells/M_FlashChip.M_FlashChip")) : nullptr;
    if (Kind != 2 && (!Star || !Slash || !Chip))
    {
        UE_LOG(LogTemp, Error, TEXT("Run Tools/create_flash_materials.py first."));
        return;
    }
    const TCHAR* Names[] = {TEXT("P_FlashDeparture"), TEXT("P_FlashArrival"), TEXT("P_ExhaustAura")};
    const FString Name = Names[Kind];
    const FString Path = TEXT("/Game/VFX/SummonerSpells/") + Name;
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
    if (Kind == 2)
    {
        // Two soft bands and inward-moving amber shards read as a sustained debuff.
        AddBurst(System, Ring, TEXT("01_AmberBinding"), 0, .85f, .85f,
            FVector(175.f, 110.f, 175.f), FVector(.65f, .22f, .015f), 0.f, 0.f, false, 1.1f, .75f, 2.4f);
        AddBurst(System, Ring, TEXT("02_InnerBinding"), 0, .65f, .65f,
            FVector(105.f, 150.f, 105.f), FVector(.85f, .35f, .025f), 0.f, 0.f, false, 1.0f, .65f, 2.f);
        AddBurst(System, Spark, TEXT("03_WeakeningShards"), 0, .4f, .7f,
            FVector(4.f, 24.f, 4.f), FVector(.9f, .30f, .015f), 68.f, -1.0f, true, 1.f, .1f, 42.f);
        AddBurst(System, Flash, TEXT("04_AmberMotes"), 0, .5f, .8f,
            FVector(6.f), FVector(.6f, .12f, .008f), 55.f, -.4f, false, .4f, .8f, 20.f);
    }
    else
    {
        const bool Arrival = Kind == 1;
        const FVector Lemon(1.f, .94f, .015f);
        // A sharp diagonal snap and angular fragments, without an expanding ring.
        AddBurst(System, Star, TEXT("01_WhiteSnap"), 1, .105f, .105f,
            FVector(Arrival ? 280.f : 230.f), FVector(1.f, 1.f, .83f), 0.f, 0.f, false, 1.f, .6f);
        AddBurst(System, Slash, TEXT("02_LemonRift"), 1, .22f, .22f,
            FVector(Arrival ? 270.f : 215.f), Lemon, 0.f, 0.f, false, 1.f, .25f);
        AddBurst(System, Chip, TEXT("03_AngularChips"), Arrival ? 20 : 12, .12f, .25f,
            FVector(7.f, 16.f, 7.f), Lemon, Arrival ? 12.f : 40.f, Arrival ? 18.f : -3.f, true, 1.f, .05f);
    }
    System->UpdateAllModuleLists();
    System->BuildEmitters();
    System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
    const bool Saved = UPackage::SavePackage(Package, System, *Filename, Args);
    UE_LOG(LogTemp, Display, TEXT("Summoner spell asset %s saved: %d"), *Name, Saved);
}
static void BuildAll() { Build(0); Build(1); Build(2); }
static void BuildFlashOnly() { Build(0); Build(1); }
static FAutoConsoleCommand FlashCommand(TEXT("LOL.Summoner.BuildFlash"), TEXT("Rebuild only Flash assets."), FConsoleCommandDelegate::CreateStatic(&BuildFlashOnly));
static FAutoConsoleCommand BuildCommand(TEXT("LOL.Summoner.BuildEffects"), TEXT("Build Flash and Exhaust particle assets."), FConsoleCommandDelegate::CreateStatic(&BuildAll));
}
#endif
