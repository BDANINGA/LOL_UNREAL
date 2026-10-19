#include "Component/JaxCounterStrikeVisualComponent.h"
#include "Component/LOL_StateComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UJaxCounterStrikeVisualComponent::UJaxCounterStrikeVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Range(TEXT("/Game/VFX/Jax/M_JaxERange.M_JaxERange"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Spin(TEXT("/Game/VFX/Jax/M_JaxESpin.M_JaxESpin"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Release(TEXT("/Game/VFX/Jax/M_JaxERelease.M_JaxERelease"));
    Plane = Mesh.Object;
    RangeMaterial = Range.Object;
    SpinMaterial = Spin.Object;
    ReleaseMaterial = Release.Object;
}

void UJaxCounterStrikeVisualComponent::AddLayer(UMaterialInterface* Material)
{
    UStaticMeshComponent* Layer = NewObject<UStaticMeshComponent>(GetOwner());
    Layer->SetupAttachment(GetOwner()->GetRootComponent());
    Layer->SetMobility(EComponentMobility::Movable);
    Layer->SetStaticMesh(Plane);
    Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Layer->SetGenerateOverlapEvents(false);
    Layer->SetCanEverAffectNavigation(false);
    Layer->SetCastShadow(false);
    Layer->bReceivesDecals = false;
    Layer->SetVisibleInRayTracing(false);
    Layer->RegisterComponent();
    Layers.Add(Layer);
    Materials.Add(Layer->CreateDynamicMaterialInstance(0, Material));
}

FVector UJaxCounterStrikeVisualComponent::FootLocation() const
{
    FVector Location = GetOwner()->GetActorLocation();
    if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
        Location.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    return Location + FVector(0, 0, 5.f);
}

void UJaxCounterStrikeVisualComponent::StartVisual(float Radius)
{
    if (GetNetMode() == NM_DedicatedServer || !Plane || !RangeMaterial || !SpinMaterial) return;
    if (!Layers.IsEmpty() && !bReleasing) return;
    ClearVisual();
    EffectRadius = FMath::Max(1.f, Radius);
    AddLayer(RangeMaterial);
    AddLayer(SpinMaterial);
    AddLayer(SpinMaterial);
    UpdateVisual();
    SetComponentTickEnabled(true);
}

void UJaxCounterStrikeVisualComponent::StopSpin()
{
    // A replicated inactive state can arrive after the release RPC; preserve its burst.
    if (!bReleasing) ClearVisual();
}

void UJaxCounterStrikeVisualComponent::ReleaseVisual(float Radius)
{
    if (GetNetMode() == NM_DedicatedServer || !Plane || !ReleaseMaterial) return;
    ClearVisual();
    EffectRadius = FMath::Max(1.f, Radius);
    bReleasing = true;
    ReleaseLocation = FootLocation();
    AddLayer(ReleaseMaterial);
    UpdateVisual();
    SetComponentTickEnabled(true);
}

void UJaxCounterStrikeVisualComponent::UpdateVisual()
{
    const FVector Foot = bReleasing ? ReleaseLocation : FootLocation();
    // The 100 cm plane's r=.94 outer edge maps to the exact gameplay radius.
    const float Scale = 2.f * EffectRadius / (100.f * .94f);
    const float Fade = bReleasing ? FMath::Pow(FMath::Clamp(1.f - Age / .4f, 0.f, 1.f), .65f)
        : FMath::Clamp(Age / .08f, 0.f, 1.f);
    for (int32 Index = 0; Index < Layers.Num(); ++Index)
    {
        UStaticMeshComponent* Layer = Layers[Index];
        const bool bGround = bReleasing || Index == 0;
        const float Height = bGround ? 0.f : (Index == 1 ? 175.f : 130.f);
        const float LayerScale = bGround ? Scale : Scale * (Index == 1 ? .62f : .50f);
        Layer->SetWorldLocationAndRotation(Foot + FVector(0, 0, Height),
            FRotator(bGround ? 0.f : (Index == 1 ? 8.f : -12.f), Index == 2 ? 140.f : 0.f, 0.f));
        Layer->SetWorldScale3D(FVector(LayerScale, LayerScale, 1.f));
        Layer->SetVisibility(!GetOwner()->IsHidden());
        if (Materials[Index])
        {
            Materials[Index]->SetScalarParameterValue(TEXT("Age"), Age + (Index == 2 ? .14f : 0.f));
            Materials[Index]->SetScalarParameterValue(TEXT("Fade"), Fade * (Index == 2 ? .5f : 1.f));
        }
    }
}

void UJaxCounterStrikeVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const ULOL_StateComponent* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>();
    if (State && State->HasStatusTag(LOLTags::State_Dead))
    {
        ClearVisual();
        return;
    }
    Age += DeltaTime;
    if (bReleasing && Age >= .4f)
    {
        ClearVisual();
        return;
    }
    UpdateVisual();
}

void UJaxCounterStrikeVisualComponent::ClearVisual()
{
    for (UStaticMeshComponent* Layer : Layers)
        if (IsValid(Layer)) Layer->DestroyComponent();
    Layers.Reset();
    Materials.Reset();
    Age = 0.f;
    bReleasing = false;
    SetComponentTickEnabled(false);
}

void UJaxCounterStrikeVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearVisual();
    Super::EndPlay(EndPlayReason);
}
