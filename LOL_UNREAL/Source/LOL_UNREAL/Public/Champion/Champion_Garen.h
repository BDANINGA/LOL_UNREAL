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
    virtual bool IsMoveInputBlocked() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(Replicated)
    bool bIsSpinning = false;



protected:
    virtual int32 GetAM_Atk_Idx() override;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q();

    void EndQBuff();

    UPROPERTY(ReplicatedUsing=OnRep_QEmpowered)
    bool bQEmpowered = false;

    UFUNCTION()
    void OnRep_QEmpowered();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ShowQImpact(ACharacter* Target, FVector_NetQuantize Location);

    UPROPERTY()
    TObjectPtr<class UMaterialInterface> QBladeMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> QImpactMaterial;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> QBladeInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> PreviousBladeOverlay;

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

    UPROPERTY(ReplicatedUsing=UpdateWShieldVisual)
    bool bWActive = false;
    FChampionStat W_OriginalStat;
    FTimerHandle W_BuffTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_DefaultDuration = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_ArmorBonus = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float W_SpellBlockBonus = 30.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Garen | W | Visual")
    TObjectPtr<class UStaticMeshComponent> WShieldComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | W | Visual")
    FVector WShieldRelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | W | Visual")
    FRotator WShieldRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | W | Visual")
    FVector WShieldRelativeScale = FVector(2.5f, 2.5f, 2.5f);

    UPROPERTY(EditDefaultsOnly, Category = "Garen | W | Visual")
    float WShieldOrbitDegreesPerSecond = 165.f;

    UFUNCTION()
    void UpdateWShieldVisual();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_E();

    UPROPERTY(Transient)
    TObjectPtr<class UGarenJudgmentTrailComponent> EBladeTrail;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> EBladeTrailMaterial;

    void ApplyEDamageTick();
    void EndESpin();

    FTimerHandle E_TickTimerHandle;
    int32 E_CurrentTick = 0;
    int32 E_MaxTicks = 0;
    float E_DamagePerTick = 0.0f;
    TMap<TWeakObjectPtr<AActor>, int32> E_HitCounts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    int32 E_TotalHits = 7;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_DefaultDuration = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_DefaultRadius = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ADRatio = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    int32 E_ArmorReductionHitCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ArmorReductionRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float E_ArmorReductionDuration = 6.0f;

    TSet<TWeakObjectPtr<AActor>> E_ArmorReducedTargets;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_R(AActor* TargetActor);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SpawnREffect(FVector SpawnLocation);

    void SpawnREffect(FVector SpawnLocation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float R_DefaultRange = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | Skills")
    float R_MissingHPRatio = 0.25f;

    UPROPERTY()
    TObjectPtr<class UNiagaraSystem> REffectSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | R | Visual")
    FVector REffectLocationOffset = FVector(0.0f, 0.0f, 20.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | R | Visual")
    FVector REffectScale = FVector(3.0f, 3.0f, 10.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | R | Visual")
    FRotator REffectRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Garen | R | Visual")
    float REffectLifeTime = 3.0f;
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
