#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FizzEWaterVisualComponent.generated.h"

/** Short world-space splash and expanding surface wave; no gameplay collision. */
UCLASS()
class LOL_UNREAL_API UFizzEWaterVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFizzEWaterVisualComponent();
    void PlaySplash(bool bLanding, FVector Location, float Radius);
    void ClearVisual();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TSoftObjectPtr<class UMaterialInterface> RippleMaterial;
    UPROPERTY() TSoftObjectPtr<class UParticleSystem> TakeoffEffect;
    UPROPERTY() TSoftObjectPtr<class UParticleSystem> LandingEffect;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Ripple;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> RippleMID;
    UPROPERTY(Transient) TObjectPtr<class UParticleSystemComponent> Splash;
    float Age = 0.f;
    float Lifetime = .9f;
};
