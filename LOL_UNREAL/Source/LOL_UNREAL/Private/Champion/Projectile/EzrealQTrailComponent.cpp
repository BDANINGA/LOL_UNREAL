#include "Champion/Projectile/EzrealQTrailComponent.h"
#include "Engine/World.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"

void UEzrealQTrailComponent::InitializeTrail(UParticleSystem* Template, float Scale, float MaxFlightTime)
{
    AActor* Missile = GetOwner();
    if (!Template || !Missile || !Missile->GetRootComponent() || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TrailEmitter = GetWorld()->SpawnActor<AEmitter>(Missile->GetActorLocation(), Missile->GetActorRotation(), Params);
    if (!TrailEmitter)
    {
        return;
    }

    TrailEmitter->SetReplicates(false); // Each client already receives the projectile multicast.
    TrailEmitter->bDestroyOnSystemFinish = true;
    TrailEmitter->AttachToComponent(Missile->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    TrailEmitter->SetActorScale3D(FVector(FMath::Clamp(Scale, 0.1f, 4.0f)));
    UParticleSystemComponent* Particles = TrailEmitter->GetParticleSystemComponent();
    Particles->SetCastShadow(false);
    Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Particles->SetTemplate(Template);
    Particles->ActivateSystem(true);
    // Also bound lifetime when the projectile owner/world disappears unexpectedly.
    TrailEmitter->SetLifeSpan(FMath::Max(MaxFlightTime, 0.2f) + 1.0f);
}

void UEzrealQTrailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TrailEmitter))
    {
        if (EndPlayReason == EEndPlayReason::Destroyed)
        {
            TrailEmitter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            TrailEmitter->GetParticleSystemComponent()->DeactivateSystem();
            TrailEmitter->SetLifeSpan(0.8f);
        }
        else
        {
            TrailEmitter->Destroy();
        }
        TrailEmitter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
