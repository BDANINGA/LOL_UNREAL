#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VayneCondemnProjectile.generated.h"

class ACharacter;
class AChampion_Vayne;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Targeted E bolt. Only the server applies the hit; each peer draws the same flight.
UCLASS()
class LOL_UNREAL_API AVayneCondemnProjectile : public AActor
{
    GENERATED_BODY()
public:
    AVayneCondemnProjectile();
    void Initialize(AChampion_Vayne* Shooter, ACharacter* InTarget);
    static void SpawnWallImpact(UWorld* World, FVector Position, FVector Normal);
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Bolt;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GlowLayers;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
    UPROPERTY() TObjectPtr<class UMaterialInterface> Glow;
    UPROPERTY(Replicated) TObjectPtr<ACharacter> Target;
    UPROPERTY(Replicated) FVector LaunchOrigin = FVector::ZeroVector;
    UPROPERTY(Replicated) float FlightDuration = .2f;
    UPROPERTY(Replicated) float LaunchServerTime = 0.f;
    UPROPERTY(Replicated) FVector ImpactPosition = FVector::ZeroVector;
    // 0: flight, 1: target hit, 2: wall hit. Kept alive to replicate the impact.
    UPROPERTY(ReplicatedUsing=OnRep_Impact) uint8 Phase = 0;
    UFUNCTION() void OnRep_Impact();
    void UpdateVisual(float DeltaSeconds);
    void BeginImpact(uint8 InPhase, FVector Position);
    float ServerTime() const;
    float LocalAge = 0.f;
    float ImpactAge = 0.f;
    uint8 VisualPhase = 0;
    bool bVisualsReady = false;
    FVector PushDirection = FVector::ForwardVector;
};
