#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FizzRWaterVisualComponent.generated.h"

/** Cosmetic travel, attached warning and eruption driven by the existing server R timer. */
UCLASS()
class LOL_UNREAL_API UFizzRWaterVisualComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFizzRWaterVisualComponent();
    void Start(FVector From, FVector To, float Travel, float Delay, float Radius, AActor* Target, FVector FallbackLocation, float FinScale, FRotator FinRotation);
    void Erupt(FVector Location);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    void Clear();
    FVector GroundAt(FVector Location) const;
    class UStaticMeshComponent* MakeMesh(class UStaticMesh* Mesh, class UMaterialInterface* Material, FName Name);
    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TSoftObjectPtr<class UStaticMesh> FinAsset;
    UPROPERTY() TSoftObjectPtr<class UMaterialInterface> FinMaterial;
    UPROPERTY() TSoftObjectPtr<class UMaterialInterface> WakeMaterial;
    UPROPERTY() TSoftObjectPtr<class UMaterialInterface> WarningMaterial;
    UPROPERTY() TSoftObjectPtr<class UParticleSystem> SprayAsset;
    UPROPERTY() TSoftObjectPtr<class UParticleSystem> BurstAsset;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Fin;
    UPROPERTY(Transient) TObjectPtr<class UStaticMeshComponent> Surface;
    UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> SurfaceMID;
    UPROPERTY(Transient) TObjectPtr<class UParticleSystemComponent> Spray;
    TWeakObjectPtr<AActor> AttachedTarget;
    FVector StartPoint, EndPoint, LastCenter, FallbackPoint;
    FRotator Heading, MeshRotation;
    float Age=0, TravelTime=0, WarningTime=2, ExplosionRadius=300, BurstAge=0;
    bool bArmed=false, bErupting=false;
};
