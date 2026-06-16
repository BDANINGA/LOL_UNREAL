#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Tryndamere.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Tryndamere : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Tryndamere();

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
	void Server_Skill_Q();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E(FVector TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayTryndamereSkillAnimation(
		uint8 SkillIndex,
		float PlayRate,
		FRotator FacingRotation
	);

	void UpdateESpin(float DeltaTime);
	void FinishESpin();
	void EndWDebuff(TWeakObjectPtr<ABaseChampion> Target);
	void EndUndyingRage();
	void BeginMovementLock(float Duration);
	void EndMovementLock();

	FVector ClampTargetLocation(FVector TargetLocation, float MaxRange) const;
	float GetSkillValue(const TArray<float>& Values, int32 Index, float Fallback) const;
	bool HasCastData(const struct FSkillData& SkillData) const;
	UAnimMontage* GetTryndamereMontage(uint8 SkillIndex) const;
	float GetAnimationDuration(uint8 SkillIndex, float Fallback) const;

	bool bESpinning = false;
	bool bMovementLocked = false;
	bool bUndyingRageActive = false;

	FVector EStartLocation = FVector::ZeroVector;
	FVector ETargetLocation = FVector::ZeroVector;
	float EElapsed = 0.0f;
	float Fury = 0.0f;

	TSet<TWeakObjectPtr<ABaseChampion>> EHitTargets;
	TMap<TWeakObjectPtr<ABaseChampion>, FTimerHandle> WDebuffTimers;
	TMap<TWeakObjectPtr<ABaseChampion>, float> WAttackDamageReductions;
	TMap<TWeakObjectPtr<ABaseChampion>, float> WOriginalMoveSpeeds;

	FTimerHandle MovementLockTimerHandle;
	FTimerHandle RTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Passive")
	float MaxFury = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Passive")
	float FuryOnHit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Passive")
	float FuryOnSkillHit = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Q")
	float QHealAbilityPowerRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Q")
	float QHealPerFury = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | Q")
	float QMissingHealthHealRatio = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | W")
	float WRadius = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | W")
	float WAttackDamageReductionRatio = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | W")
	float WSlowRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | W")
	float WDebuffDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | E")
	float EBonusADRatio = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | E")
	float EAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | E")
	float EDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | E")
	float EHitRadius = 170.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | R")
	float RDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryndamere | R")
	float RMinimumHP = 1.0f;
};
