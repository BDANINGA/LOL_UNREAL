#include "Component/LOL_RecallVisualComponent.h"
#include "Component/LOL_StateComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ULOL_RecallVisualComponent::ULOL_RecallVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SigilAsset(TEXT("/Game/VFX/Recall/M_RecallSigil.M_RecallSigil"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColumnAsset(TEXT("/Game/VFX/Recall/M_RecallColumn.M_RecallColumn"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MoteAsset(TEXT("/Game/VFX/Recall/M_RecallMote.M_RecallMote"));
    Plane = PlaneAsset.Object;
    SigilMaterial = SigilAsset.Object;
    ColumnMaterial = ColumnAsset.Object;
    MoteMaterial = MoteAsset.Object;
}

UStaticMeshComponent* ULOL_RecallVisualComponent::AddLayer(UMaterialInterface* Material)
{
    UStaticMeshComponent* Layer = NewObject<UStaticMeshComponent>(GetOwner());
    Layer->SetMobility(EComponentMobility::Movable);
    Layer->SetupAttachment(GetOwner()->GetRootComponent());
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
    return Layer;
}

FVector ULOL_RecallVisualComponent::GetFootLocation() const
{
    FVector Location = GetOwner()->GetActorLocation();
    if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        Location.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }
    Location.Z += 4.f;
    return Location;
}

void ULOL_RecallVisualComponent::StartVisual(float RecallSeconds)
{
    if (GetNetMode() == NM_DedicatedServer || !Plane || !SigilMaterial || !ColumnMaterial || !MoteMaterial) return;
    ClearVisual();
    Age = 0.f;
    Duration = FMath::Max(RecallSeconds, 0.1f);
    ReleaseAge = 0.f;
    bReleasing = false;
    bArrival = false;
    AddLayer(SigilMaterial); // Ground seal.
    AddLayer(SigilMaterial); // A lighter seal rises in the final two seconds.
    for (int32 Index = 0; Index < 3; ++Index) AddLayer(ColumnMaterial);
    for (int32 Index = 0; Index < 8; ++Index) AddLayer(MoteMaterial);
    UpdateVisual();
    SetComponentTickEnabled(true);
}

void ULOL_RecallVisualComponent::StopVisual(bool bCompleted)
{
    if (Layers.IsEmpty() || bReleasing) return;
    bReleasing = true;
    bArrival = bCompleted;
    ReleaseAge = 0.f;
    // Cancellation fades at the departure point; completion briefly seals the arrival point.
    ReleaseLocation = GetFootLocation();
    UpdateVisual();
}

void ULOL_RecallVisualComponent::UpdateVisual()
{
    const float Progress = FMath::Clamp(Age / Duration, 0.f, 1.f);
    const float Charge = FMath::SmoothStep(0.55f, 1.f, Progress);
    const float FadeIn = FMath::SmoothStep(0.f, 0.45f, Age);
    const float ReleaseDuration = bArrival ? 0.45f : 0.18f;
    const float Fade = bReleasing ? FadeIn * (1.f - FMath::Clamp(ReleaseAge / ReleaseDuration, 0.f, 1.f)) : FadeIn;
    const FVector Foot = bReleasing ? ReleaseLocation : GetFootLocation();
    const float Diameter = 3.4f * (0.83f + FadeIn * 0.17f);
    const float ArrivalScale = bArrival ? 1.f + ReleaseAge * 1.5f : 1.f;
    const bool bVisible = !GetOwner()->IsHidden();

    for (int32 Index = 0; Index < Layers.Num(); ++Index)
    {
        UStaticMeshComponent* Layer = Layers[Index];
        UMaterialInstanceDynamic* Material = Materials[Index];
        float LayerFade = Fade;
        if (Index == 0)
        {
            Layer->SetWorldLocationAndRotation(Foot, FRotator(0.f, Age * 5.f, 0.f));
            Layer->SetWorldScale3D(FVector(Diameter * ArrivalScale, Diameter * ArrivalScale, 1.f));
        }
        else if (Index == 1)
        {
            LayerFade *= Charge * 0.32f;
            const float Rise = 15.f + Charge * 230.f;
            Layer->SetWorldLocationAndRotation(Foot + FVector(0, 0, Rise), FRotator(0.f, -Age * 11.f, 0.f));
            Layer->SetWorldScale3D(FVector(Diameter * 0.7f, Diameter * 0.7f, 1.f));
        }
        else if (Index < 5)
        {
            // Three two-sided planes form restrained volume without hiding the champion.
            LayerFade *= FMath::SmoothStep(0.35f, 1.1f, Age);
            if (bArrival) LayerFade *= 0.3f;
            Layer->SetWorldLocationAndRotation(Foot + FVector(0, 0, 210.f), FRotator(0.f, (Index - 2) * 60.f + Age * 7.f, 90.f));
            Layer->SetWorldScale3D(FVector(2.3f, 4.2f, 1.f));
        }
        else
        {
            const float Phase = (Index - 5) / 8.f;
            const float Travel = FMath::Frac(Age * 0.28f + Phase);
            const float Angle = Phase * 2.f * PI + Age * 0.45f;
            const float Radius = 100.f + 22.f * FMath::Sin(Phase * 9.f + Age);
            LayerFade *= FMath::Sin(Travel * PI) * 0.75f;
            Layer->SetWorldLocationAndRotation(Foot + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 15.f + Travel * 250.f),
                FRotator(55.f, -45.f, Age * 20.f));
            Layer->SetWorldScale3D(FVector(0.16f, 0.16f, 1.f));
        }
        Layer->SetVisibility(bVisible && LayerFade > 0.001f);
        if (Material)
        {
            Material->SetScalarParameterValue(TEXT("Age"), Age);
            Material->SetScalarParameterValue(TEXT("Charge"), Charge);
            Material->SetScalarParameterValue(TEXT("Fade"), LayerFade);
        }
    }
}

void ULOL_RecallVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (const ULOL_StateComponent* State = GetOwner()->FindComponentByClass<ULOL_StateComponent>())
    {
        if (State->HasStatusTag(LOLTags::State_Dead))
        {
            ClearVisual();
            return;
        }
    }
    Age += DeltaTime;
    if (bReleasing)
    {
        ReleaseAge += DeltaTime;
        if (ReleaseAge >= (bArrival ? 0.45f : 0.18f))
        {
            ClearVisual();
            return;
        }
    }
    // Defensive cleanup if an actor loses relevance before its stop RPC arrives.
    else if (Age > Duration + 2.f)
    {
        ClearVisual();
        return;
    }
    UpdateVisual();
}

void ULOL_RecallVisualComponent::ClearVisual()
{
    for (UStaticMeshComponent* Layer : Layers)
    {
        if (IsValid(Layer)) Layer->DestroyComponent();
    }
    Layers.Reset();
    Materials.Reset();
    SetComponentTickEnabled(false);
}

void ULOL_RecallVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearVisual();
    Super::EndPlay(EndPlayReason);
}
