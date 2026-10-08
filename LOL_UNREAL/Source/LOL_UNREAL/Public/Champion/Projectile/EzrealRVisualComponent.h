#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EzrealRVisualComponent.generated.h"

/** Cosmetic R wave. Collision, damage and movement stay on the existing projectile. */
UCLASS()
class LOL_UNREAL_API UEzrealRVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEzrealRVisualComponent();
    void InitializeVisual(class UStaticMeshComponent* Mesh, class UStaticMesh* Plane,
        class UMaterialInterface* Material, class UParticleSystem* Trail, float FlightTime, float Width, float Intensity);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> WaveMaterial;
    UPROPERTY(Transient)
    TObjectPtr<class AEmitter> TrailEmitter;
    float Age = 0.f;
    float Duration = 1.f;
    float Brightness = 1.f;
};
