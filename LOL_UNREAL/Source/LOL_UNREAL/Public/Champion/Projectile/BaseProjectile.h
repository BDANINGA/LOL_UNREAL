// 롤 챔피언 투사체
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

UCLASS()
class LOL_UNREAL_API ABaseProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseProjectile();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere)
    class UNiagaraComponent* NiagaraComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UProjectileMovementComponent* ProjectileMovement;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FireAtTarget(AActor* TargetActor);

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void Deactivate();
    void Activate(FVector SpawnLocation, AActor* Target);

    void SetShooter(class AActor* Actor);
    void SetMesh(UStaticMesh* InMesh);
    void SetMeshTransform(FVector InScale, FRotator InRelativeRotation);
    void SetNiagara(class UNiagaraSystem* InNiagara);

    UPROPERTY(ReplicatedUsing = OnRep_IsActive)
    bool bIsActive = false;
protected:
	virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_IsActive();

    UFUNCTION()
    void OnRep_ProjectileVisual();

    void ApplyActiveState();
    void ApplyProjectileVisual();

    UPROPERTY()
    AActor* Shooter;

    UPROPERTY()
    AActor* CurrentTarget;

    UPROPERTY(ReplicatedUsing = OnRep_ProjectileVisual)
    TObjectPtr<UStaticMesh> ReplicatedMesh;

    UPROPERTY(ReplicatedUsing = OnRep_ProjectileVisual)
    FVector ReplicatedMeshScale = FVector::OneVector;

    UPROPERTY(ReplicatedUsing = OnRep_ProjectileVisual)
    FRotator ReplicatedMeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
};
