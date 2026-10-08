#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EzrealQTrailComponent.generated.h"

class AEmitter;
class UParticleSystem;

/** Owns a separate cosmetic emitter so existing particles can fade after the missile dies. */
UCLASS()
class LOL_UNREAL_API UEzrealQTrailComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void InitializeTrail(UParticleSystem* Template, float Scale, float MaxFlightTime);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AEmitter> TrailEmitter;
};
