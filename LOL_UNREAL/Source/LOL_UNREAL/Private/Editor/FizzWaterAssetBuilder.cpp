// Editor-only authoring for Fizz Q water sheets, spray and foam.
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
#include "Particles/Acceleration/ParticleModuleAcceleration.h"
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

namespace FizzWaterAssets
{
template<typename T> T* Module(UParticleSystem* System)
{
    auto* Result = NewObject<T>(System, NAME_None, RF_Transactional);
    Result->LODValidity = 1;
    return Result;
}

void AddLayer(UParticleSystem* System, UMaterialInterface* Material, FName Name,
    int32 Count, float LifeMin, float LifeMax, FVector Size, FVector Color,
    float Radius, float RadialVelocity, bool bStretched, float StartScale, float EndScale, float SpawnRate, bool bTrail, FVector VelocityMin, FVector VelocityMax, float Gravity = 0.f)
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
        Drift->Min = VelocityMin;
        Drift->Max = VelocityMax;
        Velocity->bInWorldSpace = false;
        Velocity->StartVelocity.Distribution = Drift;
        LOD->Modules.Add(Velocity);
    }

    if (Gravity != 0.f)
    {
        auto* Accel = Module<UParticleModuleAcceleration>(System);
        auto* Value = NewObject<UDistributionVectorConstant>(Accel);
        Value->Constant = FVector(0,0,Gravity);
        Accel->Acceleration.Distribution = Value;
        Accel->bAlwaysInWorldSpace = true;
        LOD->Modules.Add(Accel);
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
    auto* Sheet = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/VFX/Fizz/M_FizzQWaterSheet.M_FizzQWaterSheet"));
    auto* Drop = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/VFX/Fizz/M_FizzQDroplet.M_FizzQDroplet"));
    auto* Foam = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/VFX/Fizz/M_FizzQFoam.M_FizzQFoam"));
    if (!Sheet || !Drop || !Foam) return;
    const FString Name=Kind == 1 ? TEXT("P_FizzWWeapon") : Kind == 2 ? TEXT("P_FizzWImpact") : TEXT("P_FizzQWater");
    const FString Path=TEXT("/Game/VFX/Fizz/")+Name;
    UPackage* Package=CreatePackage(*Path);
    Package->FullyLoad();
    auto* System=FindObject<UParticleSystem>(Package,*Name);
    if (!System) System=NewObject<UParticleSystem>(Package,*Name,RF_Public|RF_Standalone);
    System->Modify(); System->Emitters.Reset();
    System->LODDistances={0.f}; System->LODSettings.SetNum(1);
    System->WarmupTime=0.f; System->bUseFixedRelativeBoundingBox=false;
    if (Kind == 0)
    {
    // Local crest travels with the body; world-space droplets and foam stay in its wake.
    AddLayer(System,Sheet,TEXT("01_TurquoiseBowWave"),2,.15f,.22f,FVector(115,160,115),FVector(1),
        24,0,false,.65f,1.1f,22,false,FVector(-50,-20,10),FVector(-20,20,35));
    AddLayer(System,Sheet,TEXT("02_WaterWake"),3,.22f,.34f,FVector(90,190,90),FVector(.65,.9,1),
        26,.2f,true,.8f,1.15f,28,true,FVector(-170,-30,15),FVector(-90,30,60));
    AddLayer(System,Drop,TEXT("03_LeftSpray"),8,.25f,.42f,FVector(9,21,9),FVector(1.2),
        22,.2f,true,1,.25f,75,true,FVector(-150,-230,120),FVector(-40,-90,240),-650);
    AddLayer(System,Drop,TEXT("04_RightSpray"),8,.25f,.42f,FVector(9,21,9),FVector(1.2),
        22,.2f,true,1,.25f,75,true,FVector(-150,90,120),FVector(-40,230,240),-650);
    AddLayer(System,Foam,TEXT("05_FoamWake"),3,.26f,.42f,FVector(65,45,65),FVector(.8,1,1),
        28,0,false,.55f,1.4f,26,true,FVector(-90,-35,-10),FVector(-30,35,20));
    }
    else if (Kind == 1)
    {
        AddLayer(System,Sheet,TEXT("01_TridentWaterWrap"),2,.3f,.45f,FVector(42,70,42),FVector(1.3),
            9,.15f,false,.65f,1.05f,10,false,FVector(-8,-8,5),FVector(8,8,16));
        AddLayer(System,Drop,TEXT("02_TridentDroplets"),5,.3f,.5f,FVector(5,9,5),FVector(1.4),
            16,.4f,true,.8f,.2f,20,false,FVector(-8,-8,10),FVector(8,8,35));
        AddLayer(System,Foam,TEXT("03_TridentFoam"),1,.5f,.65f,FVector(55),FVector(1.1),
            7,0,false,.45f,1.0f,3,false,FVector(-3,-3,0),FVector(3,3,5));
    }
    else
    {
        AddLayer(System,Sheet,TEXT("01_WaterCut"),7,.2f,.35f,FVector(105,145,105),FVector(1.3),
            16,5,true,.55f,1.2f,0,true,FVector(-70,-150,20),FVector(70,150,100));
        AddLayer(System,Drop,TEXT("02_ImpactSpray"),30,.25f,.46f,FVector(8,20,8),FVector(1.4),
            22,6,true,1,.15f,0,true,FVector(-140,-140,50),FVector(140,140,210),-550);
        AddLayer(System,Foam,TEXT("03_FoamBurst"),3,.28f,.42f,FVector(120),FVector(1.1),
            10,1,false,.3f,1.6f,0,true,FVector(-15,-15,0),FVector(15,15,15));
    }
    System->UpdateAllModuleLists(); System->BuildEmitters(); System->PostEditChange();
    Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    const FString File=FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension());
    UE_LOG(LogTemp,Display,TEXT("Fizz water saved: %d"),UPackage::SavePackage(Package,System,*File,Args));
}
static void BuildQ() { Build(0); }
static void BuildW() { Build(1); Build(2); }
static FAutoConsoleCommand BuildWCommand(TEXT("LOL.Fizz.BuildWWater"),TEXT("Build Fizz W weapon and impact water."),FConsoleCommandDelegate::CreateStatic(&BuildW));
static FAutoConsoleCommand BuildCommand(TEXT("LOL.Fizz.BuildQWater"),TEXT("Build Fizz Q water spray."),FConsoleCommandDelegate::CreateStatic(&BuildQ));
}
#endif