#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Fizz.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Fizz : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Fizz();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnBasicAttackHit(ACharacter* Target) override;
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_Q(ABaseChampion* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E(FVector TargetLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R(FVector TargetLocation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayFizzSkillAnimation(
		uint8 SkillIndex,
		int32 MontageIndex,
		float PlayRate,
		FRotator FacingRotation
	);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnSharkEffect(FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnRProjectile(
		FVector StartLocation,
		FVector EndLocation,
		float TravelTime
	);

	void UpdateQChaseToCast();
	void UpdateQDash(float DeltaTime);
	void FinishQDash();
	void EndWEmpower();
	void StartWPassiveBleed(ACharacter* Target);
	void BeginEDescent();
	void FinishPlayfulTrickster();
	void ExplodeChumTheWaters();
	FVector ClampTargetLocation(FVector TargetLocation, float MaxRange) const;
	float GetSkillValue(const TArray<float>& Values, int32 Index, float Fallback) const;
	float GetQSkillRange() const;
	UAnimMontage* GetFizzMontage(uint8 SkillIndex, int32 MontageIndex) const;

	UPROPERTY()
	TObjectPtr<ABaseChampion> ReservedQTarget;

	UPROPERTY()
	TObjectPtr<ABaseChampion> QDashTarget;

	UPROPERTY()
	TObjectPtr<ABaseChampion> RAttachedTarget;

	UPROPERTY()
	TObjectPtr<class USkeletalMesh> SharkMesh;

	UPROPERTY()
	TObjectPtr<class UAnimSequence> SharkAnimation;

	UPROPERTY()
	TObjectPtr<class UStaticMesh> RThrowMesh;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> RThrowMaterial;

	bool bIsChasingForQ = false;
	bool bIsQDashing = false;
	bool bWEmpowered = false;
	bool bEActive = false;
	bool bEDescending = false;

	FVector QDashStart = FVector::ZeroVector;
	FVector ETargetLocation = FVector::ZeroVector;
	FVector RExplosionLocation = FVector::ZeroVector;
	float QDashElapsed = 0.0f;

	TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> WBleedTimers;
	TMap<TWeakObjectPtr<ACharacter>, int32> WBleedTicks;

	FTimerHandle WEmpowerTimerHandle;
	FTimerHandle EAscentTimerHandle;
	FTimerHandle EDescentTimerHandle;
	FTimerHandle RExplosionTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | Q")
	float QDashDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | Q")
	float QBehindTargetDistance = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | Q")
	float QAbilityPowerRatio = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | W")
	float WActiveAbilityPowerRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | W")
	float WPassiveAbilityPowerRatio = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | W")
	int32 WPassiveTickCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | E")
	float EAbilityPowerRatio = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | E")
	float EDamageRadius = 330.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | E")
	float EDescentDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RAbilityPowerRatio = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RProjectileRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RProjectileSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RProjectileVisualRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RProjectileScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	FRotator RProjectileRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RExplosionRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RStunDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RSharkZOffset = -20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fizz | R")
	float RSharkScale = 1.0f;
};
