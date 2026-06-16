#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_LeeSin.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_LeeSin : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_LeeSin();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;
	virtual void Tick(float DeltaTime) override;
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
	void Server_Skill_W(FVector TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R(ABaseChampion* Target);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayLeeSinSkillAnimation(
		uint8 SkillIndex,
		int32 MontageIndex,
		float PlayRate,
		FRotator FacingRotation
	);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnQProjectile(
		FVector StartLocation,
		FVector EndLocation,
		float TravelTime
	);

	void MarkQTarget(ABaseChampion* Target);
	void ClearQMark();
	void StartQDash(ABaseChampion* Target);
	void UpdateQDash(float DeltaTime);
	void FinishQDash();
	void BeginSkillMovementLock(float Duration);
	void EndSkillMovementLock();
	float GetSkillMovementLockDuration(
		uint8 SkillIndex,
		int32 MontageIndex,
		float Fallback
	) const;
	void EndWShield();
	void RestoreTargetMoveSpeed(TWeakObjectPtr<ABaseChampion> Target);
	void UpdateRKnockback(float DeltaTime);
	void CheckRKnockbackCollision();
	void FinishRKnockbackCollision();
	FVector ClampTargetLocation(FVector TargetLocation, float MaxRange) const;
	float GetSkillValue(const TArray<float>& Values, int32 Index, float Fallback) const;
	bool HasCastData(const struct FSkillData& SkillData) const;
	UAnimMontage* GetLeeSinMontage(uint8 SkillIndex, int32 MontageIndex) const;

	UPROPERTY()
	TObjectPtr<ABaseChampion> QMarkedTarget;

	UPROPERTY()
	TObjectPtr<ABaseChampion> QDashTarget;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> DefaultQProjectileMesh;

	UPROPERTY()
	TObjectPtr<ABaseChampion> RKnockbackTarget;

	bool bQMarkActive = false;
	bool bIsQDashing = false;
	bool bRKnockbackActive = false;
	bool bSkillMovementLocked = false;
	FVector QDashStart = FVector::ZeroVector;
	FVector RKnockbackStart = FVector::ZeroVector;
	FVector RKnockbackEnd = FVector::ZeroVector;
	FVector RKnockbackLastLocation = FVector::ZeroVector;
	float QDashElapsed = 0.0f;
	float RKnockbackElapsed = 0.0f;
	float WShieldRemaining = 0.0f;

	FTimerHandle QMarkTimerHandle;
	FTimerHandle SkillMovementLockTimerHandle;
	FTimerHandle WShieldTimerHandle;
	FTimerHandle RCollisionCheckTimerHandle;
	FTimerHandle RCollisionEndTimerHandle;
	TMap<TWeakObjectPtr<ABaseChampion>, FTimerHandle> ESlowTimerHandles;
	TSet<TWeakObjectPtr<ABaseChampion>> RAirborneTargets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileRadius = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileVisualRadius = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QMarkDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QDashDuration = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QBonusADRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QMissingHealthMaxBonusRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | W")
	float WAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | W")
	float WShieldDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | E")
	float EBonusADRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | E")
	float ESlowRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | E")
	float ESlowDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RBonusADRatio = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RKnockbackDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RKnockbackDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RKnockbackArcHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RCollisionRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RCollisionAirborneVelocity = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | R")
	float RCollisionAirborneDuration = 0.8f;
};
