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
	ChampionName = TEXT("Blitzcrank");
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
    // �������� ���� Ÿ�̸ӵ��� �����ϰ� ����
    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().ClearTimer(W_SlowTimerHandle);

    if (GetCharacterMovement())
    {
        // ������ �ӵ��� �ٱ��� Ŭ���̾�Ʈ �����Ʈ�� �ڵ� ����ȭ(Replication)�˴ϴ�.
        GetCharacterMovement()->MaxWalkSpeed = 330.0f + (330.0f * W_SpeedBuffAmount);
    }

    UE_LOG(LogTemp, Log, TEXT("[Server Blitz W] ���� �ߵ�"));

    // ���� ���� Ÿ�̸� �Ŵ����� ���� ���� Ÿ�̸� ���
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
    UE_LOG(LogTemp, Log, TEXT("[Server Blitz W] ��ȭ ����, ���� ����"));
}

// ==========================================
// E ��ų : ��ö �ָ� (���� �Է� -> ���� ����)
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

    // [��Ÿ ĵ�� ó��] �������� ���� ��ٿ� Ÿ�̸Ӹ� �����Ͽ� ��� ���� ���� ����
    // GetWorldTimerManager().ClearTimer(AttackCooldownHandle);

    UE_LOG(LogTemp, Log, TEXT("[Server Blitz E] ��ö �ָ� ���� Ȱ��ȭ"));
}

void AChampion_Blitz::OnAttackHitWithE(ABaseChampion* Target)
{
    
}

void AChampion_Blitz::ResetE()
{
    bIsEActive = false;
}

void AChampion_Blitz::Skill_R() {}