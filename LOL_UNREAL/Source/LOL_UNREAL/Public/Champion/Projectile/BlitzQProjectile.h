#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlitzQProjectile.generated.h"

class AChampion_Blitz;
class UStaticMesh;

// The server moves and sweeps this projectile; clients display its replicated position.
UCLASS()
class LOL_UNREAL_API ABlitzQProjectile : public AActor
{
    GENERATED_BODY()

public:
    ABlitzQProjectile();
    virtual void Tick(float DeltaTime) override;

    void Initialize(AChampion_Blitz* InShooter, float InSpeed, float InRange, float InRadius,
        UStaticMesh* InMesh, float InVisualRadius, float InVisualScale, FRotator InRotationOffset);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UStaticMeshComponent> MeshComponent;

    UPROPERTY(ReplicatedUsing = OnRep_Visual)
    TObjectPtr<UStaticMesh> VisualMesh;

    UPROPERTY(ReplicatedUsing = OnRep_Visual)
    float VisualScale = 1.0f;

    UPROPERTY(ReplicatedUsing = OnRep_Visual)
    FRotator VisualRotation = FRotator::ZeroRotator;

    UFUNCTION()
    void OnRep_Visual();

    TWeakObjectPtr<AChampion_Blitz> Shooter;
    FVector Direction = FVector::ForwardVector;
    float Speed = 1200.0f;
    float RemainingDistance = 0.0f;
    float Radius = 50.0f;
    bool bFinished = false;
};
