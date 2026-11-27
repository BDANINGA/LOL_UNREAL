#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"

class UWorld;

namespace LOLSkillCollision
{
    // Sweeps a circle in XY against combat bodies, ignoring elevation and sensors.
    // Returns one hit per live body, ordered by first contact along the segment.
    // Team and skill-specific target restrictions remain with the caller.
    LOL_UNREAL_API bool Sweep(UWorld* World, TArray<FHitResult>& Hits,
        const FVector& Start, const FVector& End, float Radius,
        const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam);
}
