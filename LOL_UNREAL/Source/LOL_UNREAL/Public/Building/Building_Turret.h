#pragma once

#include "CoreMinimal.h"
#include "Building/BaseBuilding.h"
#include "Building_Turret.generated.h"

UCLASS()
class LOL_UNREAL_API ABuilding_Turret : public ABaseBuilding
{
	GENERATED_BODY()
public:
	ABuilding_Turret();

	class UNiagaraSystem* GetAllyProjectileNiagara() { return  AllyProjectileNiagara; };
	class UNiagaraSystem* GetEnemyProjectileNiagara() { return  EnemyProjectileNiagara; };

	virtual void OnBuildingDeath() override;
	USkeletalMeshComponent* GetDestructionMesh() const;

private:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<class UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<class USkeletalMeshComponent> DestructionMesh;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	class UNiagaraSystem* AllyProjectileNiagara;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	class UNiagaraSystem* EnemyProjectileNiagara;
};
