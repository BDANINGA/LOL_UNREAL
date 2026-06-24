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
    virtual void OnBasicAttackHit(ACharacter* Target) override;


protected:
    // --- Q 천천히 끌어오기 관련 변수 및 함수 선언 ---
    FTimerHandle PullTimerHandle;
    FTimerHandle PullTimeoutTimerHandle;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q(FVector TargetLocation);

    UPROPERTY()
    class ACharacter* GrabbedTarget;

    ECollisionEnabled::Type GrabbedTargetPreviousCollisionEnabled =
        ECollisionEnabled::QueryAndPhysics;
    ECollisionResponse GrabbedTargetPreviousPawnResponse = ECR_Block;

    // 숫자가 낮을수록 더 천천히 끌려옵니다 (테스트 후 조절 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Skills")
    float PullSpeed = 10.0f;

    void TickPullTarget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Skills")
    float Q_Range = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Skills")
    float Q_Radius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Skills")
    float Q_PullDistance = 100.0f;

    FVector PullDestination;

    UPROPERTY()
    TObjectPtr<class UStaticMesh> QProjectileMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Projectile")
    float QProjectileSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Projectile")
    float QProjectileVisualRadius = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Projectile")
    float QProjectileVisualScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Projectile")
    FRotator QProjectileRotationOffset = FRotator::ZeroRotator;

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_SpawnQProjectileVisual(FVector StartLocation, FVector EndLocation, float TravelTime);

    void SpawnQProjectileVisual(FVector StartLocation, FVector EndLocation, float TravelTime);
    void BeginPullTarget(ACharacter* Target, FVector SkillDirection);
    void FinishPullTarget();
    void RestoreGrabbedTargetCollision();
    bool IsValidBlitzSkillTarget(AActor* TargetActor) const;

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
    float E_AirborneDuration = 1.0f;
    float E_DamageMultiplier = 2.0f;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_E();

    UPROPERTY()
    bool bIsEActive = false;

    UFUNCTION()
    void ResetE();

    void OnAttackHitWithE(ACharacter* Target);

    // --- R 스킬 (서버 실행) ---
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_R();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blitz | Skills")
    float R_APRatio = 1.0f;
};
