#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Jax.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Jax : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Jax();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnBasicAttackHit(ACharacter* Target) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	bool IsWEmpowered() const { return bWEmpowered; }
	UAnimMontage* GetWEmpoweredAttackMontage() const;
	float GetWEmpoweredAttackPlayRate() const { return WEmpoweredAttackPlayRate; }

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_Q(ACharacter* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayJaxSkillAnimation(uint8 SkillIndex, float PlayRate, FRotator FacingRotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetGrandmastersMightIdle(bool bEnabled);

	void UpdateQChaseToCast();
	void UpdateLeap(float DeltaTime);
	void UpdateGrandmastersMightIdleAnimation();
	void FinishLeap();
	void EndWEmpower();
	void FinishCounterStrike();
	void EndGrandmastersMight();
	void ApplyEmpowerDamage(ACharacter* Target);
	void ApplyGrandmastersMightActiveDamage();
	void ApplyGrandmastersMightPassive(ACharacter* Target);
	bool IsValidLeapStrikeTarget(AActor* Target) const;
	float GetQSkillRange() const;
	UAnimMontage* GetSkillMontage(uint8 SkillIndex) const;

	UPROPERTY()
	TObjectPtr<class UAnimSequence> FallbackSkillAnimation;

	UPROPERTY()
	TObjectPtr<UAnimMontage> GrandmastersMightIdleMontage;

	UPROPERTY()
	TObjectPtr<ACharacter> ReservedQTarget;

	UPROPERTY()
	TObjectPtr<ACharacter> LeapTarget;

	bool bIsChasingForQ = false;
	bool bIsLeaping = false;
	bool bWEmpowered = false;
	bool bCounterStrikeActive = false;
	bool bGrandmastersMightActive = false;
	bool bGrandmastersMightIdleEnabled = false;

	FVector LeapStart = FVector::ZeroVector;
	float LeapElapsed = 0.0f;
	int32 CounterStrikeDodgedAttacks = 0;
	int32 GrandmastersMightHitCount = 0;
	float RArmorBonusApplied = 0.0f;
	float RSpellBlockBonusApplied = 0.0f;

	FTimerHandle WTimerHandle;
	FTimerHandle ETimerHandle;
	FTimerHandle RDamageTimerHandle;
	FTimerHandle UltTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | Q")
	float QLeapDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | Q")
	float QBonusADRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | W")
	float WEmpowerDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | W")
	float WEmpoweredAttackPlayRate = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | E")
	float EAbilityPowerRatio = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | E")
	float ETargetMaxHPRatio = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | E")
	float EDodgedAttackDamageRatio = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | E")
	int32 EMaxDamageStacks = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | E")
	float EStunDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | R")
	int32 RPassiveHitThreshold = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | R")
	float RBaseArmorBonus = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | R")
	float RBaseSpellBlockBonus = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | R")
	float RBonusADToArmorRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jax | R")
	float RAPToSpellBlockRatio = 0.2f;
};
