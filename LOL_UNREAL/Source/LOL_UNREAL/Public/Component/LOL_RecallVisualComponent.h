#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_RecallVisualComponent.generated.h"

/** Local cosmetic layers, driven by the champion's authoritative recall RPCs. */
UCLASS()
class LOL_UNREAL_API ULOL_RecallVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULOL_RecallVisualComponent();
    void StartVisual(float RecallSeconds);
    void StopVisual(bool bCompleted);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ClearVisual();
    void UpdateVisual();
    FVector GetFootLocation() const;
    class UStaticMeshComponent* AddLayer(class UMaterialInterface* Material);

    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TObjectPtr<class UMaterialInterface> SigilMaterial;
    UPROPERTY() TObjectPtr<class UMaterialInterface> ColumnMaterial;
    UPROPERTY() TObjectPtr<class UMaterialInterface> MoteMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Layers;
    UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> Materials;

    float Age = 0.f;
    float Duration = 8.f;
    float ReleaseAge = 0.f;
    bool bReleasing = false;
    bool bArrival = false;
    FVector ReleaseLocation = FVector::ZeroVector;
};
