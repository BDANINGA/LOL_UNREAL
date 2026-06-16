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
    void SetNiagara(class UNiagaraSystem* InNiagara);

    bool bIsActive = false;
protected:
	virtual void BeginPlay() override;

private:
    UPROPERTY()
    AActor* Shooter;
};
