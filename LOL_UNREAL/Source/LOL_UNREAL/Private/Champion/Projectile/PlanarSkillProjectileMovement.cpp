#include "Champion/Projectile/PlanarSkillProjectileMovement.h"
#include "Combat/SkillCollision.h"

void UPlanarSkillProjectileMovement::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    AActor* Projectile = GetOwner();
    if (!IsValid(Projectile) || !UpdatedComponent) return;
    const FVector Start = UpdatedComponent->GetComponentLocation();
    const float Speed = Velocity.Size2D();
    const float StepTime = Speed > KINDA_SMALL_NUMBER
        ? FMath::Min(FMath::Max(DeltaTime, 0.f), RemainingDistance / Speed) : 0.f;
    Super::TickComponent(StepTime, TickType, ThisTickFunction);
    if (!IsValid(Projectile) || !UpdatedComponent) return;
    const FVector End = UpdatedComponent->GetComponentLocation();
    RemainingDistance = FMath::Max(0.f, RemainingDistance - static_cast<float>(FVector::Dist2D(Start, End)));

    if (Projectile->HasAuthority())
    {
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Projectile);
        Params.AddIgnoredActor(Projectile->GetOwner());
        TArray<FHitResult> Hits;
        LOLSkillCollision::Sweep(GetWorld(), Hits, Start, End, CollisionRadius, Params);
        for (const FHitResult& Hit : Hits)
        {
            OnSkillHit.Broadcast(Hit);
            if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed()) return;
        }
    }
    if (RemainingDistance <= KINDA_SMALL_NUMBER) Projectile->Destroy();
}
