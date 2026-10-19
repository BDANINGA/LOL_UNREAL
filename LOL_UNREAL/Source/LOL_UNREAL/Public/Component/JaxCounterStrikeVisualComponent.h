#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JaxCounterStrikeVisualComponent.generated.h"

/** Cosmetic range indicator, overhead spinning ribbons and a short release wave. */
UCLASS()
class LOL_UNREAL_API UJaxCounterStrikeVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UJaxCounterStrikeVisualComponent();
    void StartVisual(float Radius);
    void StopSpin();
    void ReleaseVisual(float Radius);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void ClearVisual();
    void UpdateVisual();
    FVector FootLocation() const;
    void AddLayer(class UMaterialInterface* Material);
    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TObjectPtr<class UMaterialInterface> RangeMaterial;
    UPROPERTY() TObjectPtr<class UMaterialInterface> SpinMaterial;
    UPROPERTY() TObjectPtr<class UMaterialInterface> ReleaseMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Layers;
    UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> Materials;
    float Age = 0.f;
    float EffectRadius = 300.f;
    bool bReleasing = false;
    FVector ReleaseLocation = FVector::ZeroVector;
};
