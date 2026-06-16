#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Component/LOL_StatComponent.h"
#include "Champion_Garen.generated.h"


UCLASS()
class LOL_UNREAL_API AChampion_Garen : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Garen();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

	virtual void OnBasicAttackHit(ACharacter* Target) override;

    virtual void Tick(float DeltaTime) override;

    UPROPERTY()
    bool bIsSpinning = false;



protected:
    virtual int32 GetAM_Atk_Idx() override;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q();

    void EndQBuff();

    bool bQEmpowered = false;

    FTimerHandle Q_BuffTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float Q_MoveSpeedRatio = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float Q_ADRatio = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float Q_DefaultDuration = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float Q_DefaultSilenceDuration = 1.5f;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_W();

    void EndWBuff();

    bool bWActive = false;
    FChampionStat W_OriginalStat;
    FTimerHandle W_BuffTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_DefaultDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_ArmorBonus = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_SpellBlockBonus = 30.0f;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_E();

    void ApplyEDamageTick();
    void EndESpin();

    FTimerHandle E_TickTimerHandle;
    int32 E_CurrentTick = 0;
    int32 E_MaxTicks = 0;
    float E_DamagePerTick = 0.0f;
    TMap<TWeakObjectPtr<ABaseChampion>, int32> E_HitCounts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    int32 E_TotalHits = 7;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_DefaultDuration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_DefaultRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ADRatio = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    int32 E_ArmorReductionHitCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ArmorReductionRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ArmorReductionDuration = 6.0f;

    TSet<TWeakObjectPtr<ABaseChampion>> E_ArmorReducedTargets;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_R(AActor* TargetActor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float R_DefaultRange = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float R_MissingHPRatio = 0.25f;
	FTimerHandle UltTimerHandle;

    UPROPERTY()
    ABaseChampion* ReservedRTarget = nullptr;

    bool bIsChasingForR = false;

    void UpdateRChaseToCast();
    float GetRSkillRange();

    bool bIsCastingR = false;

    FTimerHandle R_CastLockTimerHandle;

    void EndRCastLock();
};
