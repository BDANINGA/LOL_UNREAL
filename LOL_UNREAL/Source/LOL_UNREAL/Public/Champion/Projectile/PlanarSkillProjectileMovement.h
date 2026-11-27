#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PlanarSkillProjectileMovement.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPlanarSkillHit, const FHitResult&);

// Visual movement stays at the launch height; gameplay contacts use the XY plane.
UCLASS()
class LOL_UNREAL_API UPlanarSkillProjectileMovement : public UProjectileMovementComponent
{
    GENERATED_BODY()

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    float CollisionRadius = 0.f;
    float RemainingDistance = 0.f;
    FPlanarSkillHit OnSkillHit;
};
