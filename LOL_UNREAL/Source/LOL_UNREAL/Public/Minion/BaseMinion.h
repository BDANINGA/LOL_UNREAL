// 미니언 기본 뼈대
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseMinion.generated.h"

UCLASS()
class LOL_UNREAL_API ABaseMinion : public APawn
{
	GENERATED_BODY()

public:
	ABaseMinion();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Pawn")
	class UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Pawn")
	class USkeletalMeshComponent* MeshComponent;

	// ------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_StatComponent* StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_AttackComponent* AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_MoveComponent* MoveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_LifeCycleComponent* LifeCycleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_UIComponent* UIComponent;

	// -------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	AActor* CombatTarget;

	FORCEINLINE class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

	virtual FName GetMinionName() const { return MinionName; };

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Minion|Data")
	FName MinionName = TEXT("Minion_Melee");
};
