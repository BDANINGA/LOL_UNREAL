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

    //2026 05 04 수정
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SetCondemnRotation(FRotator NewRotation);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_SpawnArrow(FVector TargetLocation);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
   // 2026 05 13
   // ------------- R스킬 관련 함수들 -------------
   // 서버 실행 및 멀티캐스트
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_R();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayRMontage();

    // 궁극기 종료
    void End_Skill_R();

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Q")
    float Q_BonusDamageRatio = 500.8f;  // 추가 피해 계수 (공격력 × 0.8)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Q")
    float Q_EmpowerDuration = 6.0f;  // 강화 평타 유지 시간 (못 쓰면 만료)

    UPROPERTY(Replicated)
    bool bQEmpowered = false;  // 다음 평타 강화 상태

    FTimerHandle Q_EmpoweredTimerHandle;

    void EndQEmpower();  // 강화 만료 처리

    // --- W ��ų ���� ���� ---

    /** 추가 피해 발동에 필요한 평타 횟수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    int32 BoltsThreshold = 3;

    /** 3타째 추가 고정 피해량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    float BoltsBonusDamage = 100.0f;

    /** 일정 시간 같은 적을 안 때리면 스택 초기화 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|W")
    float BoltsStackDuration = 3.0f;

    /** 타겟별 평타 누적 스택 */
    TMap<TWeakObjectPtr<ACharacter>, int32> SilverBoltsStack;

    /** 타겟별 스택 만료 타이머 핸들 */
    TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> SilverBoltsTimers;

    /** 3타째 발동: 추가 피해 + 이펙트 */
    void TriggerSilverBolts(ACharacter* Target);

    // 2026 05 12 평타 강화데미지
    virtual void OnBasicAttackHit(ACharacter* Target) override;

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

    // ------------- R스킬 관련 함수들 -------------
    // 궁극기 활성화 여부 (블루프린트에서 애니메이션 전환 등에 활용 가능)
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Vayne")
    bool bIsFinalHourActive;

    // 궁극기 데이터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Vayne")
    float R_Duration = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Vayne")
    float R_BonusAD = 35.0f;

    // 지속 시간 관리를 위한 타이머 핸들
    FTimerHandle R_TimerHandle;
};
