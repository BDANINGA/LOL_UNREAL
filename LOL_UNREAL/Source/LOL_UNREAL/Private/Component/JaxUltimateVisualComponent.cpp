#include "Component/JaxUltimateVisualComponent.h"
#include "Component/LOL_StateComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UJaxUltimateVisualComponent::UJaxUltimateVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Armor(TEXT("/Game/VFX/Jax/M_JaxRArmor.M_JaxRArmor"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Crest(TEXT("/Game/VFX/Jax/M_JaxRCrest.M_JaxRCrest"));
    Sphere = SphereAsset.Object;
    Plane = PlaneAsset.Object;
    ArmorMaterial = Armor.Object;
    CrestMaterial = Crest.Object;
}

void UJaxUltimateVisualComponent::AddLayer(UStaticMesh* Mesh, UMaterialInterface* Material)
{
    auto* Layer = NewObject<UStaticMeshComponent>(GetOwner());
    Layer->SetupAttachment(GetOwner()->GetRootComponent());
    Layer->SetMobility(EComponentMobility::Movable);
    Layer->SetStaticMesh(Mesh);
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

void UJaxUltimateVisualComponent::StartVisual()
{
    if (GetNetMode() == NM_DedicatedServer || !Layers.IsEmpty() || !Sphere || !Plane || !ArmorMaterial || !CrestMaterial) return;
    const auto* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>();
    if (State && State->HasStatusTag(LOLTags::State_Dead)) return;
    Age = 0.f;
    AddLayer(Sphere, ArmorMaterial);
    AddLayer(Plane, CrestMaterial);
    UpdateVisual();
    SetComponentTickEnabled(true);
}

void UJaxUltimateVisualComponent::UpdateVisual()
{
    FVector Foot = GetOwner()->GetActorLocation();
    if (const auto* Character = Cast<ACharacter>(GetOwner()))
        Foot.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    // Compact crest is a buff marker, not an ability-range indicator.
    const float Strength = 1.f + .55f * FMath::Exp(-Age * 5.f);
    for (int32 Index = 0; Index < Layers.Num(); ++Index)
    {
        const bool bArmor = Index == 0;
        Layers[Index]->SetWorldLocationAndRotation(Foot + FVector(0, 0, bArmor ? 120.f : 5.f), FRotator::ZeroRotator);
        Layers[Index]->SetWorldScale3D(bArmor ? FVector(2.05f, 2.05f, 2.6f) : FVector(2.5f, 2.5f, 1.f));
        Layers[Index]->SetVisibility(!GetOwner()->IsHidden());
        if (Materials[Index])
        {
            Materials[Index]->SetScalarParameterValue(TEXT("Age"), Age);
            Materials[Index]->SetScalarParameterValue(TEXT("Fade"), Strength);
        }
    }
}

void UJaxUltimateVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>();
    if (State && State->HasStatusTag(LOLTags::State_Dead))
    {
        StopVisual();
        return;
    }
    Age += DeltaTime;
    UpdateVisual();
}

void UJaxUltimateVisualComponent::StopVisual()
{
    for (UStaticMeshComponent* Layer : Layers)
        if (IsValid(Layer)) Layer->DestroyComponent();
    Layers.Reset();
    Materials.Reset();
    Age = 0.f;
    SetComponentTickEnabled(false);
}

void UJaxUltimateVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopVisual();
    Super::EndPlay(EndPlayReason);
}
