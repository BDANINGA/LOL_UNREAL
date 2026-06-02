#include "Champion/Champion_Garen.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Garen::AChampion_Garen()
{
    ChampionName = TEXT("Garen");
    SetChampionData(ChampionName);
}

void AChampion_Garen::Skill_Q()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_Q();
}

bool AChampion_Garen::Server_Skill_Q_Validate() { return true; }

void AChampion_Garen::Server_Skill_Q_Implementation()
{
    if (!SkillComponent) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;
    const float BuffDuration = QData.Duration.IsValidIndex(SkillLevelIdx) ? QData.Duration[SkillLevelIdx] : Q_DefaultDuration;

    bQEmpowered = true;

    if (GetCharacterMovement() && StatComponent)
    {
        const float BaseMoveSpeed = StatComponent->GetStat().MoveSpeed > 0.f ? StatComponent->GetStat().MoveSpeed : 330.0f;
        GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * (1.0f + Q_MoveSpeedRatio);
    }

    GetWorldTimerManager().ClearTimer(Q_BuffTimerHandle);
    GetWorldTimerManager().SetTimer(Q_BuffTimerHandle, this, &AChampion_Garen::EndQBuff, BuffDuration, false);
}

void AChampion_Garen::EndQBuff()
{
    bQEmpowered = false;

    if (GetCharacterMovement() && StatComponent)
    {
        const float BaseMoveSpeed = StatComponent->GetStat().MoveSpeed > 0.f ? StatComponent->GetStat().MoveSpeed : 330.0f;
        GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
    }
}

void AChampion_Garen::OnBasicAttackHit(ACharacter* Target)
{
    if (!HasAuthority()) return;
    if (!bQEmpowered) return;
    if (!IsValid(Target) || Target == this) return;
    if (!SkillComponent || !StatComponent) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target);
    if (!TargetChampion) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;

    const float BaseDamage = QData.BaseDamage.IsValidIndex(SkillLevelIdx) ? QData.BaseDamage[SkillLevelIdx] : 0.0f;
    const float SilenceDuration = QData.SecondaryValue.IsValidIndex(SkillLevelIdx) ? QData.SecondaryValue[SkillLevelIdx] : Q_DefaultSilenceDuration;
    const float BonusDamage = BaseDamage + StatComponent->GetStat().AttackDamage * Q_ADRatio;

    if (BonusDamage > 0.f)
    {
        UGameplayStatics::ApplyDamage(
            Target,
            BonusDamage,
            GetController(),
            this,
            ULOL_DamagePhysical::StaticClass()
        );
    }

    TargetChampion->Multicast_ApplySilence(SilenceDuration);

    bQEmpowered = false;
    GetWorldTimerManager().ClearTimer(Q_BuffTimerHandle);

    if (GetCharacterMovement() && StatComponent)
    {
        const float BaseMoveSpeed = StatComponent->GetStat().MoveSpeed > 0.f ? StatComponent->GetStat().MoveSpeed : 330.0f;
        GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
    }
}

void AChampion_Garen::Skill_W()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_W();
}

bool AChampion_Garen::Server_Skill_W_Validate() { return true; }

void AChampion_Garen::Server_Skill_W_Implementation()
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("W", 1)) return;

    FSkillData& WData = SkillComponent->GetW_Data();
    const int32 SkillLevelIdx = 0;
    const float Duration = WData.Duration.IsValidIndex(SkillLevelIdx) ? WData.Duration[SkillLevelIdx] : W_DefaultDuration;
    const float ArmorBonus = WData.SecondaryValue.IsValidIndex(0) ? WData.SecondaryValue[0] : W_ArmorBonus;
    const float SpellBlockBonus = WData.SecondaryValue.IsValidIndex(1) ? WData.SecondaryValue[1] : W_SpellBlockBonus;

    if (!bWActive)
    {
        W_OriginalStat = StatComponent->GetStat();
    }

    FChampionStat BuffedStat = W_OriginalStat;
    BuffedStat.Armor += ArmorBonus;
    BuffedStat.SpellBlock += SpellBlockBonus;

    bWActive = true;
    StatComponent->SetStat(BuffedStat);

    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().SetTimer(W_BuffTimerHandle, this, &AChampion_Garen::EndWBuff, Duration, false);
}

void AChampion_Garen::EndWBuff()
{
    if (!bWActive || !StatComponent) return;

    bWActive = false;
    StatComponent->SetStat(W_OriginalStat);
}

void AChampion_Garen::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_E();
}

bool AChampion_Garen::Server_Skill_E_Validate() { return true; }

void AChampion_Garen::Server_Skill_E_Implementation()
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    FSkillData& EData = SkillComponent->GetE_Data();
    const int32 SkillLevelIdx = 0;

    const float Duration = EData.Duration.IsValidIndex(SkillLevelIdx) ? EData.Duration[SkillLevelIdx] : E_DefaultDuration;
    const float TotalBaseDamage = EData.BaseDamage.IsValidIndex(SkillLevelIdx) ? EData.BaseDamage[SkillLevelIdx] : 0.0f;
    const float TotalDamage = TotalBaseDamage + StatComponent->GetStat().AttackDamage * E_ADRatio;

    E_CurrentTick = 0;
    E_MaxTicks = FMath::Max(1, FMath::RoundToInt(Duration / E_TickInterval));
    E_DamagePerTick = TotalDamage / E_MaxTicks;

    GetWorldTimerManager().ClearTimer(E_TickTimerHandle);
    GetWorldTimerManager().SetTimer(E_TickTimerHandle, this, &AChampion_Garen::ApplyEDamageTick, E_TickInterval, true);

    ApplyEDamageTick();
}

void AChampion_Garen::ApplyEDamageTick()
{
    if (!HasAuthority() || !SkillComponent || !StatComponent)
    {
        EndESpin();
        return;
    }

    FSkillData& EData = SkillComponent->GetE_Data();
    const int32 SkillLevelIdx = 0;
    const float Radius = EData.Range.IsValidIndex(SkillLevelIdx) ? EData.Range[SkillLevelIdx] : E_DefaultRadius;

    TArray<FHitResult> Hits;
    const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (bHit && E_DamagePerTick > 0.f)
    {
        TSet<ABaseChampion*> HitChampions;

        for (const FHitResult& Hit : Hits)
        {
            ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
            if (!TargetChampion || TargetChampion == this || HitChampions.Contains(TargetChampion)) continue;
            if (TargetChampion->StateComponent->HasStatusTag(LOLTags::State_Dead)) continue;

            HitChampions.Add(TargetChampion);

            UGameplayStatics::ApplyDamage(
                TargetChampion,
                E_DamagePerTick,
                GetController(),
                this,
                ULOL_DamagePhysical::StaticClass()
            );
        }
    }

    E_CurrentTick++;
    if (E_CurrentTick >= E_MaxTicks)
    {
        EndESpin();
    }
}

void AChampion_Garen::EndESpin()
{
    GetWorldTimerManager().ClearTimer(E_TickTimerHandle);
    E_CurrentTick = 0;
    E_MaxTicks = 0;
    E_DamagePerTick = 0.0f;
}

void AChampion_Garen::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_R();
}

bool AChampion_Garen::Server_Skill_R_Validate() { return true; }

void AChampion_Garen::Server_Skill_R_Implementation()
{
    /*if (!SkillComponent || !StatComponent) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(CombatTarget);
    if (!TargetChampion || TargetChampion == this) return;
    if (!TargetChampion->StatComponent) return;
    if (TargetChampion->StateComponent->HasStatusTag(LOLTags::State_Dead)) return;

    FSkillData& RData = SkillComponent->GetR_Data();
    const int32 SkillLevelIdx = 0;

    const float Range = RData.Range.IsValidIndex(SkillLevelIdx) ? RData.Range[SkillLevelIdx] : R_DefaultRange;
    if (GetDistanceTo(TargetChampion) > Range) return;

    if (!SkillComponent->TryCastSkill("R", 1)) return;

    const float BaseDamage = RData.BaseDamage.IsValidIndex(SkillLevelIdx) ? RData.BaseDamage[SkillLevelIdx] : 0.0f;
    const float MaxHP = TargetChampion->StatComponent->GetStat().MaxHP;
    const float CurrentHP = TargetChampion->StatComponent->GetCurrentHP();
    const float MissingHP = FMath::Max(0.0f, MaxHP - CurrentHP);
    const float MissingHPRatio = RData.SecondaryValue.IsValidIndex(SkillLevelIdx) ? RData.SecondaryValue[SkillLevelIdx] : R_MissingHPRatio;
    const float SkillDamage = BaseDamage + MissingHP * MissingHPRatio;

    if (SkillDamage <= 0.0f) return;

    UGameplayStatics::ApplyDamage(
        TargetChampion,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamageTrueDamage::StaticClass()
    );*/
}
