// 공격 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_AttackComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_AttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_AttackComponent();

	void UpdateAttackLogic();

	void StartAttack();
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void EndAttack();
	void ResetAttack();
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ExecuteAttackHit();
	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ExecuteRangeAttackHit();
	void CancelAttack();

	void ReceivedCrowdControl();
	void ResetAfterRespawn();

	bool CanAttack() const { return bCanAttack; }
	bool HitHappened() const { return bHitHappened; }

	void OnBasicAttackHit(ACharacter* Target);

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* HitTarget = nullptr;

	void SetCombatTarget(AActor* Target);
	bool IsValidAttackTarget(AActor* Target) const;

	FTimerHandle AttackTimerHandle;
	FTimerHandle AttackHitTimerHandle;

	bool bCanAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Timing", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float BasicAttackMontagePlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Timing", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float ChampionAttackSpeedMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Timing", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float AttackHitTimingRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Damage", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MinionAttackDamageMultiplier = 0.5f;
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Category = "Pooling")
	int32 PoolSize = 20;

	UPROPERTY()
	TArray<class ABaseProjectile*> ProjectilePool;

	class ABaseProjectile* GetProjectileFromPool();

private:
	UPROPERTY()
	class APawn* OwnerPawn;

	
	
	bool bHitHappened = false;
};
