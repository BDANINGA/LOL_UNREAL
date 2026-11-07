#include "Component/FizzRWaterVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

UFizzRWaterVisualComponent::UFizzRWaterVisualComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> P(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Plane=P.Object;
    FinAsset=TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/SM_FizzRFin.SM_FizzRFin")));
    FinMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/M_FizzRFin.M_FizzRFin")));
    WakeMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/M_FizzRWake.M_FizzRWake")));
    WarningMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/M_FizzRWarning.M_FizzRWarning")));
    SprayAsset=TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/P_FizzRWake.P_FizzRWake")));
    BurstAsset=TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/P_FizzELanding.P_FizzELanding")));
}

UStaticMeshComponent* UFizzRWaterVisualComponent::MakeMesh(UStaticMesh* Mesh, UMaterialInterface* Material, FName Name)
{
    if (!Mesh || !Material) return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),UStaticMeshComponent::StaticClass(),Name));
    C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(Mesh); C->SetMaterial(0,Material);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false); C->SetCastShadow(false); C->bReceivesDecals=false;
    C->SetVisibleInRayTracing(false); C->RegisterComponent();
    return C;
}

FVector UFizzRWaterVisualComponent::GroundAt(FVector Location) const
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FizzRSurface),false,GetOwner());
    if (GetWorld()->LineTraceSingleByObjectType(Hit,Location+FVector(0,0,200),Location-FVector(0,0,800),
        FCollisionObjectQueryParams(ECC_WorldStatic),Params)) return Hit.ImpactPoint+FVector(0,0,5);
    return Location-FVector(0,0,140);
}

void UFizzRWaterVisualComponent::Start(FVector From,FVector To,float Travel,float Delay,float Radius,AActor* Target,FVector FallbackLocation,float FinScale,FRotator FinRotation)
{
    if (GetNetMode()==NM_DedicatedServer) return;
    Clear(); StartPoint=From; EndPoint=To; LastCenter=To; AttachedTarget=Target;
    FallbackPoint=FallbackLocation; MeshRotation=FinRotation;
    TravelTime=FMath::Max(Travel,0.f); WarningTime=FMath::Max(Delay,.1f); ExplosionRadius=FMath::Max(Radius,1.f);
    Heading=(To-From).Rotation(); Heading.Pitch=Heading.Roll=0;
    Fin=MakeMesh(FinAsset.LoadSynchronous(),FinMaterial.LoadSynchronous(),TEXT("FizzRFin"));
    if (Fin) Fin->SetWorldScale3D(FVector(FMath::Max(FinScale,.1f)));
    Surface=MakeMesh(Plane,WakeMaterial.LoadSynchronous(),TEXT("FizzRSurface"));
    if (Surface) { SurfaceMID=Surface->CreateDynamicMaterialInstance(0); Surface->SetWorldScale3D(FVector(4.8f,2.8f,1)); }
    if (auto* Effect=SprayAsset.LoadSynchronous())
        Spray=UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Effect,GroundAt(From),Heading,FVector(1),false);
    SetComponentTickEnabled(true);
    TickComponent(0,LEVELTICK_All,nullptr);
}

void UFizzRWaterVisualComponent::Erupt(FVector Location)
{
    if (GetNetMode()==NM_DedicatedServer) return;
    if (Fin) { Fin->DestroyComponent(); Fin=nullptr; }
    if (Surface) { Surface->DestroyComponent(); Surface=nullptr; SurfaceMID=nullptr; }
    if (Spray) { Spray->DestroyComponent(); Spray=nullptr; }
    LastCenter=Location; bErupting=true; BurstAge=0;
    if (auto* Effect=BurstAsset.LoadSynchronous())
        Spray=UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),Effect,GroundAt(Location)+FVector(0,0,20),
            FRotator::ZeroRotator,FVector(ExplosionRadius/330.f),false);
    SetComponentTickEnabled(true);
}

void UFizzRWaterVisualComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime,TickType,TickFunction);
    Age+=DeltaTime;
    // Follow the attached target's visibility; an active R remains valid after its caster dies.
    const bool bVisible=AttachedTarget.IsValid() ? !AttachedTarget->IsHidden() : !GetOwner()->IsHidden();
    if (bErupting)
    {
        if (Spray) Spray->SetVisibility(bVisible);
        BurstAge+=DeltaTime; if (BurstAge>1.f) Clear();
        return;
    }
    if (Age>TravelTime+WarningTime+1.f) { Clear(); return; } // Lost/late explosion RPC safety.
    if (!bArmed && Age>=TravelTime)
    {
        bArmed=true; LastCenter=EndPoint;
        if (Spray) Spray->DeactivateSystem();
        if (Surface && WarningMaterial.LoadSynchronous())
        {
            SurfaceMID=Surface->CreateDynamicMaterialInstance(0,WarningMaterial.Get());
            Surface->SetWorldScale3D(FVector(2.f*ExplosionRadius/94.f,2.f*ExplosionRadius/94.f,1));
        }
    }
    if (bArmed)
    {
        // Match the existing server fallback if the marked target is destroyed.
        LastCenter=AttachedTarget.IsValid() ? AttachedTarget->GetActorLocation() : FallbackPoint;
    }
    else LastCenter=FMath::Lerp(StartPoint,EndPoint,FMath::Clamp(Age/FMath::Max(TravelTime,.001f),0.f,1.f));
    FVector Ground=GroundAt(LastCenter);
    if (Surface)
    {
        FVector Center=Ground;
        if (!bArmed) Center-=Heading.Vector()*110.f;
        Surface->SetWorldLocationAndRotation(Center,bArmed ? FRotator::ZeroRotator : Heading);
        Surface->SetVisibility(bVisible);
    }
    if (SurfaceMID)
    {
        SurfaceMID->SetScalarParameterValue(TEXT("Age"),Age);
        SurfaceMID->SetScalarParameterValue(TEXT("Progress"),FMath::Clamp((Age-TravelTime)/WarningTime,0.f,1.f));
    }
    if (Fin)
    {
        const float Angle=(Age-TravelTime)*2.6f;
        FVector FinPosition=Ground; FRotator Rotation=Heading;
        if (bArmed)
        {
            FinPosition+=FVector(FMath::Cos(Angle),FMath::Sin(Angle),0)*ExplosionRadius*.55f;
            Rotation.Yaw=FMath::RadiansToDegrees(Angle)+90.f;
        }
        Fin->SetWorldLocationAndRotation(FinPosition+FVector(0,0,3+FMath::Sin(Age*9)*3),(Rotation.Quaternion()*MeshRotation.Quaternion()).Rotator());
        Fin->SetVisibility(bVisible);
    }
    if (Spray)
    {
        if (!bArmed) Spray->SetWorldLocationAndRotation(Ground+FVector(0,0,16),Heading);
        Spray->SetVisibility(bVisible);
    }
}

void UFizzRWaterVisualComponent::Clear()
{
    if (Fin) Fin->DestroyComponent(); if (Surface) Surface->DestroyComponent(); if (Spray) Spray->DestroyComponent();
    Fin=nullptr; Surface=nullptr; SurfaceMID=nullptr; Spray=nullptr; AttachedTarget.Reset();
    Age=0; BurstAge=0; bArmed=false; bErupting=false; SetComponentTickEnabled(false);
}
void UFizzRWaterVisualComponent::EndPlay(const EEndPlayReason::Type Reason) { Clear(); Super::EndPlay(Reason); }
