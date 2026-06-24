#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Alistar.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Alistar : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Alistar();

	void UpdateWChaseToCast();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

	virtual void Tick(float DeltaTime) override;
	virtual bool CanCastWhileStunned(uint8 skilltype) const override;
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ClearCC();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDamageReduction = 0.7f;   // 70% Í∞êÏÜå

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_Q();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")

	TObjectPtr<class ACharacter> CurrentWTarget;
	bool bIsW_Dashing = false;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W(ACharacter* Target);

	// --- W ?§ÌÇ¨ ---
	UPROPERTY()
	ACharacter* ReservedWTarget = nullptr;

	bool bIsChasingForW = false;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|W")
	float W_CastRange = 550.0f;
	
	void ApplyWKnockback(ACharacter* Target);
	void RestoreWCollision();

	ECollisionEnabled::Type WDashPreviousCollisionEnabled =
		ECollisionEnabled::QueryAndPhysics;
	ECollisionResponse WDashPreviousPawnResponse = ECR_Block;
	// --- E ?§ÌÇ¨: Î∂ÑÏáÑ ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_Radius = 400.0f;  // ?®Í≥º Î∞òÍ≤Ω

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_TickInterval = 1.0f;  // ?∞Î?ÏßÄ Í∞ÑÍ≤© (Ï¥?

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	int32 E_MaxTicks = 5;  // Ï¥??∞Î?ÏßÄ ?üÏàò

	int32 E_CurrentTick = 0;  // ?ÑÏû¨ÍπåÏ? Î™?Î≤??∞Î?ÏßÄ Ï§¨ÎäîÏßÄ

	FTimerHandle E_TickTimerHandle;

	// ?§Ïùå ?âÌ????§ÌÑ¥ Î∂Ä???åÎûòÍ∑?
	UPROPERTY(Replicated)
	bool bNextAttackStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_StunDuration = 1.5f;  // Í∞ïÌôî ?âÌ? ?§ÌÑ¥ ?úÍ∞Ñ

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_StunBuffDuration = 5.0f;  // Í∞ïÌôî ?âÌ? ?¨Ïö© Í∞Ä???úÍ∞Ñ (???∞Î©¥ ÎßåÎ£å)

	FTimerHandle E_StunBuffTimerHandle;

	void EndStunBuff();  // Í∞ïÌôî ?âÌ? ÎßåÎ£å Ï≤òÎ¶¨

	// ?âÌ? ?ÅÏ§ë ÏΩúÎ∞± ?§Î≤Ñ?ºÏù¥??
	virtual void OnBasicAttackHit(ACharacter* Target) override;

	// ??Îß§Í∞úÎ≥Ä???úÍ±∞ (Target ??Î∞õÏùå)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E();

	void ApplyEDamageTick();
	
	// --- R ?§ÌÇ¨: Î∂àÍµ¥???òÏ? ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDuration = 7.0f;  // ÏßÄ?çÏãúÍ∞?

	UPROPERTY(Replicated)
	bool bIsUltActive = false;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|R|Visual")
	TObjectPtr<class UStaticMeshComponent> RShieldComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R|Visual")
	FVector RShieldRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R|Visual")
	FRotator RShieldRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R|Visual")
	FVector RShieldRelativeScale = FVector(0.03f, 0.03f, 0.03f);

	FTimerHandle UltTimerHandle;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayRMontage();


	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetRShieldVisible(bool bVisible);

	void StartUlt();
	void EndUlt();

	void ClearCCExceptKnockup();

	// Q ?úÏ†Ñ ?úÍ∞Ñ (?¥Îèô ?†Í∏à Í∏∞Í∞Ñ) --- 2026 05 07
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Q")
	float Q_CastTime = 0.5f;

	bool bCanAttack = true;

	FTimerHandle Q_CastTimerHandle;

	void EndQCast();
};
