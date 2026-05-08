#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h" 
#include "Champion_Vayne.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Vayne : public ABaseChampion
{
	GENERATED_BODY()

public:
    AChampion_Vayne();

    virtual void SetChampionData(FName RowName) override;

    virtual void Skill_Q() override;
    virtual void Skill_W() override;
    virtual void Skill_E() override;
    virtual void Skill_R() override;

    virtual void Tick(float DeltaTime) override;

    //2026 05 01 수정
    void OnBasicAttackHit(ACharacter* Target);

    //2026 05 04 수정
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SetCondemnRotation(FRotator NewRotation);

protected:
    // --- ���� Q ���� ���� ---

    bool bIsDashing = false;

    float DashTime = 0.2f;     // ��� ���� �ð�
    float DashElapsed = 0.0f;  // ��� �ð�

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q(FVector QLocation);
    void Server_Skill_Q_Implementation(FVector QLocation);
    bool Server_Skill_Q_Validate(FVector QLocation);

    // ★ 추가 — Q 모션 멀티캐스트
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayQMontage();

    FVector DashStart;

    FVector DashTarget;

    // --- W ��ų ���� ���� ---

    /** 추가 피해 발동에 필요한 평타 횟수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    int32 BoltsThreshold = 3;

    /** 3타째 추가 고정 피해량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    float BoltsBonusDamage = 30.0f;

    /** 일정 시간 같은 적을 안 때리면 스택 초기화 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    float BoltsStackDuration = 3.0f;

    /** 타겟별 평타 누적 스택 */
    TMap<TWeakObjectPtr<ACharacter>, int32> SilverBoltsStack;

    /** 타겟별 스택 만료 타이머 핸들 */
    TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> SilverBoltsTimers;

    /** 3타째 발동: 추가 피해 + 이펙트 */
    void TriggerSilverBolts(ACharacter* Target);

    // --- E ��ų ���� ���� ---

    // �������� ���� ó���� ������ RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ExecuteCondemn(ACharacter* Target);

    // �����Ϳ��� ���� ������ ��ų ��ġ
    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushDistance = 1000.0f; // �з��� �Ÿ�

    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushTime = 0.4f; // �з����� �ð� (��)

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayEMontage();
};
