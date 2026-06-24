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

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DestroyQProjectile();

	void MarkQTarget(AActor* Target);
	void ClearQMark();
	void UpdateQProjectile(float DeltaTime);
	void FinishQProjectile(bool bDestroyVisual);
	void ApplyQFirstHitDamage(AActor* Target);
	void StartQDash(AActor* Target);
	void UpdateQDash(float DeltaTime);
	void FinishQDash();
	void StartWDash(FVector Destination);
	void UpdateWDash(float DeltaTime);
	void FinishWDash();
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
	TObjectPtr<AActor> QMarkedTarget;

	UPROPERTY()
	TObjectPtr<AActor> QDashTarget;

	UPROPERTY()
	TObjectPtr<AActor> QProjectileVisualActor;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> DefaultQProjectileMesh;

	UPROPERTY()
	TObjectPtr<ABaseChampion> RKnockbackTarget;

	bool bQMarkActive = false;
	bool bQProjectileActive = false;
	bool bIsQDashing = false;
	bool bIsWDashing = false;
	bool bRKnockbackActive = false;
	bool bSkillMovementLocked = false;
	ECollisionEnabled::Type QDashPreviousCollisionEnabled =
		ECollisionEnabled::QueryAndPhysics;
	ECollisionResponse QDashPreviousPawnResponse = ECR_Block;
	ECollisionEnabled::Type WDashPreviousCollisionEnabled =
		ECollisionEnabled::QueryAndPhysics;
	ECollisionResponse WDashPreviousPawnResponse = ECR_Block;
	FVector QDashStart = FVector::ZeroVector;
	FVector QProjectileStart = FVector::ZeroVector;
	FVector QProjectileEnd = FVector::ZeroVector;
	FVector QProjectileLastLocation = FVector::ZeroVector;
	FVector QProjectileDirection = FVector::ZeroVector;
	FVector WDashStart = FVector::ZeroVector;
	FVector WDashEnd = FVector::ZeroVector;
	FVector RKnockbackStart = FVector::ZeroVector;
	FVector RKnockbackEnd = FVector::ZeroVector;
	FVector RKnockbackLastLocation = FVector::ZeroVector;
	float QDashElapsed = 0.0f;
	float QProjectileElapsed = 0.0f;
	float QProjectileTravelTime = 0.0f;
	float WDashElapsed = 0.0f;
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
	float QProjectileRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileVisualRadius = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QProjectileVisualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	FRotator QProjectileRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QMarkDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QDashDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QBonusADRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | Q")
	float QMissingHealthMaxBonusRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | W")
	float WAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | W")
	float WShieldDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeeSin | W")
	float WDashDuration = 0.45f;

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
