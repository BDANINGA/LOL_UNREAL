#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Blitz.generated.h"

UCLASS()
class AChampion_Blitz : public ABaseChampion
{
	GENERATED_BODY()
	
public:
    AChampion_Blitz();

    virtual void Skill_Q() override;
    virtual void Skill_W() override;
    virtual void Skill_E() override;
    virtual void Skill_R() override;

    // E스킬 활성화 상태에서 적을 때렸을 때 호출할 함수 (기본공격 판정부에서 호출 필요)
    void OnAttackHitWithE(ABaseChampion* Target);

protected:
    // --- W 스킬 (서버 실행) ---
    FTimerHandle W_BuffTimerHandle;
    FTimerHandle W_SlowTimerHandle;

    float W_Duration = 5.0f;
    float W_SlowDuration = 1.5f;
    float W_SpeedBuffAmount = 0.7f;
    float W_SlowAmount = 0.3f;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_W();

    void EndWBuff();
    void EndWSlow();

    // --- E 스킬 (서버 실행) ---
    bool bIsEActive = false;
    float E_AirborneDuration = 1.0f;
    float E_DamageMultiplier = 2.0f;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_E();

    void ResetE();
};
