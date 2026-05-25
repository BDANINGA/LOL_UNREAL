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

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

	virtual void Tick(float DeltaTime) override;
	virtual bool CanCastWhileStunned(uint8 skilltype) const override;
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ClearCC();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDamageReduction = 0.7f;   // 70% 감소

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_Q();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")

	TObjectPtr<class ACharacter> CurrentWTarget;
	bool bIsW_Dashing = false;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_W(ACharacter* Target);
	
	void ApplyWKnockback(ACharacter* Target);
	// --- E 스킬: 분쇄 ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_Radius = 400.0f;  // 효과 반경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_TickInterval = 1.0f;  // 데미지 간격 (초)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	int32 E_MaxTicks = 3;  // 총 데미지 횟수

	int32 E_CurrentTick = 0;  // 현재까지 몇 번 데미지 줬는지

	FTimerHandle E_TickTimerHandle;

	// 다음 평타에 스턴 부여 플래그
	UPROPERTY(Replicated)
	bool bNextAttackStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_StunDuration = 1.5f;  // 강화 평타 스턴 시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|E")
	float E_StunBuffDuration = 5.0f;  // 강화 평타 사용 가능 시간 (안 쓰면 만료)

	FTimerHandle E_StunBuffTimerHandle;

	void EndStunBuff();  // 강화 평타 만료 처리

	// 평타 적중 콜백 오버라이드
	virtual void OnBasicAttackHit(ACharacter* Target) override;

	// ★ 매개변수 제거 (Target 안 받음)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_E();

	void ApplyEDamageTick();  // 매 틱마다 호출되는 함수
	
	// --- R 스킬: 불굴의 의지 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDuration = 7.0f;  // 지속시간

	UPROPERTY(Replicated)
	bool bIsUltActive = false;

	FTimerHandle UltTimerHandle;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayRMontage();

	void StartUlt();
	void EndUlt();

	void ClearCCExceptKnockup();

	// Q 시전 시간 (이동 잠금 기간) --- 2026 05 07
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Q")
	float Q_CastTime = 0.5f;

	bool bCanAttack = true;

	FTimerHandle Q_CastTimerHandle;

	void EndQCast();
};
