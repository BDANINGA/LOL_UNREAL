#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Gragas.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Gragas : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Gragas();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnBasicAttackHit(ACharacter* Target) override;
	virtual bool IsMoveInputBlocked() const override;
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_Q(FVector TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E(FVector TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R(FVector TargetLocation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayGragasSkillAnimation(
		uint8 SkillIndex,
		float PlayRate,
		FRotator FacingRotation
	);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnGragasProjectile(
		uint8 ProjectileType,
		FVector StartLocation,
		FVector EndLocation,
		float TravelTime,
		float LifeTime
	);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyQProjectile();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnGragasExplosionEffect(FVector SpawnLocation, float VisualRadius);

	void SpawnGragasExplosionEffect(FVector SpawnLocation, float VisualRadius);
	void ExplodeQ();
	void EndWEmpower();
	void EndWDamageReduction();
	void UpdateEDash(float DeltaTime);
	void FinishEDash(AActor* HitTarget);
	void ExplodeR();
	void RestoreQSlow(TWeakObjectPtr<ABaseChampion> Target);
	void BeginMovementLock(float Duration);
	void EndMovementLock();

	FVector ClampTargetLocation(FVector TargetLocation, float MaxRange) const;
	float GetSkillValue(const TArray<float>& Values, int32 Index, float Fallback) const;
	bool HasCastData(const struct FSkillData& SkillData) const;
	UAnimMontage* GetGragasMontage(uint8 SkillIndex) const;
	float GetAnimationDuration(uint8 SkillIndex, float Fallback) const;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> QProjectileMesh;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> RProjectileMesh;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> FloatEffectMesh;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> ProjectileMaterial;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> FloatEffectMaterial;

	UPROPERTY()
	TObjectPtr<AActor> LocalQProjectileActor;

	bool bQActive = false;
	bool bWEmpowered = false;
	bool bWDamageReductionActive = false;
	bool bEDashing = false;
	bool bMovementLocked = false;

	FVector QExplosionLocation = FVector::ZeroVector;
	FVector EStartLocation = FVector::ZeroVector;
	FVector ETargetLocation = FVector::ZeroVector;
	FVector RExplosionLocation = FVector::ZeroVector;
	float QCastStartTime = 0.0f;
	float QTravelTime = 0.0f;
	float EElapsed = 0.0f;

	FTimerHandle QExplosionTimerHandle;
	FTimerHandle WEmpowerTimerHandle;
	FTimerHandle WDamageReductionTimerHandle;
	FTimerHandle RExplosionTimerHandle;
	FTimerHandle MovementLockTimerHandle;
	TMap<TWeakObjectPtr<ABaseChampion>, FTimerHandle> QSlowTimerHandles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QMaxChargeDamageBonus = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QFuseDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QExplosionRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QProjectileSpeed = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QProjectileVisualRadius = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QSlowRatio = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Q")
	float QSlowDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | W")
	float WAbilityPowerRatio = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | W")
	float WTargetMaxHPRatio = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | W")
	float WDamageReductionRatio = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | W")
	float WDamageReductionDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | W")
	float WEmpowerDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EAbilityPowerRatio = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EDashDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EHitRadius = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EDamageRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EStunDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EKnockbackSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | E")
	float EKnockbackDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RExplosionRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RProjectileSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RProjectileVisualRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Visual")
	float QExplosionEffectVisualRadius = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Visual")
	float RExplosionEffectVisualRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Visual")
	float ExplosionEffectLifeTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Visual")
	float ExplosionEffectHeightOffset = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | Visual")
	FRotator ExplosionEffectRotation = FRotator(90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RKnockbackSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gragas | R")
	float RKnockbackDuration = 0.45f;
};
