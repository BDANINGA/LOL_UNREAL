#include "Component/FizzEWaterVisualComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

UFizzEWaterVisualComponent::UFizzEWaterVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    Plane = Mesh.Object;
    RippleMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/M_FizzERipple.M_FizzERipple")));
    TakeoffEffect = TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/P_FizzETakeoff.P_FizzETakeoff")));
    LandingEffect = TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/VFX/Fizz/P_FizzELanding.P_FizzELanding")));
}

void UFizzEWaterVisualComponent::PlaySplash(bool bLanding, FVector Location, float Radius)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    const auto* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>();
    if (State && State->HasStatusTag(LOLTags::State_Dead)) return;
    ClearVisual();
    Lifetime = bLanding ? .9f : .6f;
    FHitResult Ground;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FizzEWaterGround), false, GetOwner());
    FRotator SurfaceRotation = FRotator::ZeroRotator;
    if (GetWorld()->LineTraceSingleByObjectType(Ground, Location + FVector(0,0,100),
        Location - FVector(0,0,200), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
    {
        Location = Ground.ImpactPoint;
        SurfaceRotation = FRotationMatrix::MakeFromZ(Ground.ImpactNormal).Rotator();
    }
    const float VisualRadius = FMath::Clamp(Radius, 20.f, 1500.f);
    if (UParticleSystem* Effect = (bLanding ? LandingEffect : TakeoffEffect).LoadSynchronous())
    {
        const float Scale = bLanding ? VisualRadius / 330.f : 1.f;
        Splash = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Effect,
            Location + FVector(0,0,24), FRotator::ZeroRotator, FVector(Scale), false);
        if (Splash) Splash->SetVisibility(!GetOwner()->IsHidden());
    }
    if (UMaterialInterface* Material = RippleMaterial.LoadSynchronous(); Material && Plane)
    {
        Ripple = NewObject<UStaticMeshComponent>(GetOwner());
        Ripple->SetMobility(EComponentMobility::Movable);
        Ripple->SetStaticMesh(Plane);
        Ripple->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Ripple->SetGenerateOverlapEvents(false);
        Ripple->SetCanEverAffectNavigation(false);
        Ripple->SetCastShadow(false);
        Ripple->bReceivesDecals = false;
        Ripple->SetVisibleInRayTracing(false);
        Ripple->RegisterComponent();
        Ripple->SetWorldLocationAndRotation(Location + FVector(0,0,4), SurfaceRotation);
        const float Scale = 2.f * VisualRadius / 94.f;
        Ripple->SetWorldScale3D(FVector(Scale,Scale,1));
        Ripple->SetVisibility(!GetOwner()->IsHidden());
        RippleMID = Ripple->CreateDynamicMaterialInstance(0, Material);
        if (RippleMID)
        {
            RippleMID->SetScalarParameterValue(TEXT("Age"), 0.f);
            RippleMID->SetScalarParameterValue(TEXT("Life"), Lifetime);
        }
    }
    SetComponentTickEnabled(true);
}

void UFizzEWaterVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    Age += DeltaTime;
    const auto* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>();
    if (Age >= Lifetime || (State && State->HasStatusTag(LOLTags::State_Dead)))
    {
        ClearVisual();
        return;
    }
    if (RippleMID) RippleMID->SetScalarParameterValue(TEXT("Age"), Age);
    if (Ripple) Ripple->SetVisibility(!GetOwner()->IsHidden());
    if (Splash) Splash->SetVisibility(!GetOwner()->IsHidden());
}

void UFizzEWaterVisualComponent::ClearVisual()
{
    if (Ripple) Ripple->DestroyComponent();
    if (Splash) Splash->DestroyComponent();
    Ripple = nullptr; RippleMID = nullptr; Splash = nullptr;
    Age = 0.f;
    SetComponentTickEnabled(false);
}

void UFizzEWaterVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearVisual();
    Super::EndPlay(EndPlayReason);
}
