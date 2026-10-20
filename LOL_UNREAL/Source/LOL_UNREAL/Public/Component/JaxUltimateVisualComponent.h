#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JaxUltimateVisualComponent.generated.h"

/** Persistent armor silhouette and foot crest while Grandmaster's Might is active. */
UCLASS()
class LOL_UNREAL_API UJaxUltimateVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UJaxUltimateVisualComponent();
    void StartVisual();
    void StopVisual();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void AddLayer(class UStaticMesh* Mesh, class UMaterialInterface* Material);
    void UpdateVisual();
    UPROPERTY() TObjectPtr<class UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TObjectPtr<class UMaterialInterface> ArmorMaterial;
    UPROPERTY() TObjectPtr<class UMaterialInterface> CrestMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Layers;
    UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> Materials;
    float Age = 0.f;
};
