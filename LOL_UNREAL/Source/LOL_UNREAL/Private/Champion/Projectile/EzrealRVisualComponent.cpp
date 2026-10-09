#include "Champion/Projectile/EzrealRVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"

UEzrealRVisualComponent::UEzrealRVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEzrealRVisualComponent::InitializeVisual(UStaticMeshComponent* Mesh, UStaticMesh* Plane,
    UMaterialInterface* Material, UParticleSystem* Trail, float FlightTime, float Width, float Intensity)
{
    if (!Mesh || !Plane || !Material || GetNetMode() == NM_DedicatedServer)
    {
        SetComponentTickEnabled(false);
        return;
    }
    Duration = FMath::Max(FlightTime + 0.1f, 0.2f);
    Brightness = FMath::Max(Intensity, 0.f);
    const float VisualScale = FMath::Clamp(Width, 100.f, 1200.f) / 600.f;
    Mesh->SetStaticMesh(Plane);
    Mesh->SetRelativeRotation(FRotator::ZeroRotator);
    Mesh->SetRelativeScale3D(FVector(4.8f, 6.f, 1.f) * VisualScale);
    Mesh->SetRelativeLocation(FVector(-60.f * VisualScale, 0.f, 0.f));
    WaveMaterial = Mesh->CreateDynamicMaterialInstance(0, Material);
    if (WaveMaterial)
    {
        WaveMaterial->SetScalarParameterValue(TEXT("WaveFade"), 0.f);
        WaveMaterial->SetScalarParameterValue(TEXT("WaveIntensity"), Brightness);
    }

    if (Trail)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        TrailEmitter = GetWorld()->SpawnActor<AEmitter>(GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), Params);
        if (TrailEmitter)
        {
            TrailEmitter->SetReplicates(false);
            TrailEmitter->bDestroyOnSystemFinish = true;
            TrailEmitter->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            TrailEmitter->SetActorScale3D(FVector(VisualScale));
            UParticleSystemComponent* Particles = TrailEmitter->GetParticleSystemComponent();
            Particles->SetCastShadow(false);
            Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Particles->SetTemplate(Trail);
            Particles->ActivateSystem(true);
            TrailEmitter->SetLifeSpan(Duration + 0.8f);
        }
    }
}

void UEzrealRVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    Age += DeltaTime;
    if (WaveMaterial)
    {
        const float Fade = FMath::Min(FMath::Clamp(Age / 0.07f, 0.f, 1.f), FMath::Clamp((Duration - Age) / 0.18f, 0.f, 1.f));
        WaveMaterial->SetScalarParameterValue(TEXT("WaveFade"), Fade);
        // A brief release flare settles into a steady, readable travelling edge.
        WaveMaterial->SetScalarParameterValue(TEXT("WaveIntensity"), Brightness * (1.f + 0.55f * FMath::Exp(-Age * 15.f)));
    }
    if (IsValid(TrailEmitter) && Age >= Duration - 0.15f)
    {
        TrailEmitter->GetParticleSystemComponent()->DeactivateSystem();
    }
}

void UEzrealRVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TrailEmitter))
    {
        if (EndPlayReason == EEndPlayReason::Destroyed)
        {
            TrailEmitter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            TrailEmitter->GetParticleSystemComponent()->DeactivateSystem();
            TrailEmitter->SetLifeSpan(0.6f);
        }
        else
        {
            TrailEmitter->Destroy();
        }
        TrailEmitter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
