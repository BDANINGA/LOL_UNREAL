#include "Champion/Champion_Blitz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h" 

#include "UObject/ConstructorHelpers.h"

AChampion_Blitz::AChampion_Blitz()
{
	ChampionName = TEXT("Blitz");
	SetChampionData(ChampionName);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 330.0f;
	}
}

void AChampion_Blitz::Skill_Q() {}
void AChampion_Blitz::Skill_W() 
{
	if (!IsLocallyControlled()) return;
	if (bIsStunned || bIsKnockedBack) return;

	Server_Skill_W();
}

bool AChampion_Blitz::Server_Skill_W_Validate() { return true; }

void AChampion_Blitz::Server_Skill_W_Implementation()
{
    // 서버에서 기존 타이머들을 안전하게 정리
    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().ClearTimer(W_SlowTimerHandle);

    if (GetCharacterMovement())
    {
        // 서버가 속도를 바구면 클라이언트 무브먼트로 자동 동기화(Replication)됩니다.
        GetCharacterMovement()->MaxWalkSpeed = 330.0f + (330.0f * W_SpeedBuffAmount);
    }

    UE_LOG(LogTemp, Log, TEXT("[Server Blitz W] 폭주 발동"));

    // 서버 월드 타이머 매니저로 버프 종료 타이머 등록
    GetWorldTimerManager().SetTimer(W_BuffTimerHandle, this, &AChampion_Blitz::EndWBuff, W_Duration, false);
}

void AChampion_Blitz::EndWBuff()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f - (330.0f * W_SlowAmount);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Server Blitz W] End W Buff - Slow Start"));

    GetWorldTimerManager().SetTimer(W_SlowTimerHandle, this, &AChampion_Blitz::EndWSlow, W_SlowDuration, false);
}

void AChampion_Blitz::EndWSlow()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f;
    }
    UE_LOG(LogTemp, Log, TEXT("[Server Blitz W] 둔화 해제, 정상 복구"));
}

// ==========================================
// E 스킬 : 강철 주먹 (로컬 입력 -> 서버 실행)
// ==========================================
void AChampion_Blitz::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_E();
}

bool AChampion_Blitz::Server_Skill_E_Validate() { return true; }

void AChampion_Blitz::Server_Skill_E_Implementation()
{
    bIsEActive = true;

    // [평타 캔슬 처리] 서버에서 공격 쿨다운 타이머를 리셋하여 즉시 다음 공격 유도
    // GetWorldTimerManager().ClearTimer(AttackCooldownHandle);

    UE_LOG(LogTemp, Log, TEXT("[Server Blitz E] 강철 주먹 버프 활성화"));
}

void AChampion_Blitz::OnAttackHitWithE(ABaseChampion* Target)
{
    
}

void AChampion_Blitz::ResetE()
{
    bIsEActive = false;
}

void AChampion_Blitz::Skill_R() {}