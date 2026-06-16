#include "Champion/Champion_Garen.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Component/LOL_AttackComponent.h"
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

    // 1. 현재 재생 중인 평타 애니메이션을 즉시 끊어버립니다.
    if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
    {
        AnimInst->StopAllMontages(0.1f);
    }

    if (GetCharacterMovement() && StatComponent)
    {
        const float BaseMoveSpeed = StatComponent->GetStat().MoveSpeed > 0.f ? StatComponent->GetStat().MoveSpeed : 330.0f;
        GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * (1.0f + Q_MoveSpeedRatio);
    }

    GetWorldTimerManager().ClearTimer(Q_BuffTimerHandle);
    GetWorldTimerManager().SetTimer(Q_BuffTimerHandle, this, &AChampion_Garen::EndQBuff, BuffDuration, false);
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

    float SkillDamage = QData.BaseDamage.IsValidIndex(0) ? QData.BaseDamage[0] : 0.0f;
    SkillDamage += StatComponent->GetStat().AttackDamage * 0.8f;
    const float SilenceDuration = QData.SecondaryValue.IsValidIndex(SkillLevelIdx) ? QData.SecondaryValue[SkillLevelIdx] : Q_DefaultSilenceDuration;

    if (SkillDamage > 0.f)
    {
        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
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

int32 AChampion_Garen::GetAM_Atk_Idx()
{
    if (bQEmpowered)
    {
        // 어택 컴포넌트가 일반 평타를 실행한 직후 바로 다음 틱에 Q 애니메이션으로 덮어씁니다.
        GetWorldTimerManager().SetTimerForNextTick([this]()
            {
                if (bQEmpowered)
                {
                    // 원하시는 멀티캐스트 재생 코드 (배율 2.0f)
                    Multicast_PlayMontage(ChampionResource.QMontage[AM_SKIll_Q_IDX], 1.0f);
                }
            });

        return Super::GetAM_Atk_Idx();
    }

    return Super::GetAM_Atk_Idx();
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

void AChampion_Garen::Skill_W()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_W();
}

bool AChampion_Garen::Server_Skill_W_Validate() { return true; }

void AChampion_Garen::Server_Skill_W_Implementation()
{
    Multicast_PlayMontage(ChampionResource.WMontage[AM_SKIll_W_IDX], 1.0f);

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

    bIsSpinning = true;

    StopAnimMontage();

    if (ULOL_AttackComponent* AttackComp = FindComponentByClass<ULOL_AttackComponent>())
    {
        AttackComp->bCanAttack = false;
        AttackComp->CombatTarget = nullptr;
        AttackComp->HitTarget = nullptr;

        GetWorldTimerManager().ClearTimer(AttackComp->AttackTimerHandle);
    }

    Multicast_PlayMontage(ChampionResource.EMontage[AM_SKIll_E_IDX], 1.0f);

    if (ULOL_AttackComponent* AttackComp = FindComponentByClass<ULOL_AttackComponent>())
    {
        AttackComp->bCanAttack = false;
    }

    E_CurrentTick = 0;
    E_MaxTicks = 7;
    E_HitCounts.Empty();
    E_ArmorReducedTargets.Empty();

    const float TickInterval = 3.0f / static_cast<float>(E_MaxTicks);

    GetWorldTimerManager().SetTimer(
        E_TickTimerHandle,
        this,
        &AChampion_Garen::ApplyEDamageTick,
        TickInterval,
        true
    );

    ApplyEDamageTick();
}

void AChampion_Garen::EndESpin()
{
    bIsSpinning = false;

    GetWorldTimerManager().ClearTimer(E_TickTimerHandle);
    E_CurrentTick = 0;

    if (ULOL_AttackComponent* AttackComp = FindComponentByClass<ULOL_AttackComponent>())
    {
        AttackComp->bCanAttack = true;
    }
}

void AChampion_Garen::ApplyEDamageTick()
{
    if (!HasAuthority() || !SkillComponent || !StatComponent)
    {
        EndESpin();
        return;
    }
    FSkillData& EData = SkillComponent->GetE_Data();

    const float Radius = 200.f;

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

    TArray<ABaseChampion*> Targets;
    TSet<ABaseChampion*> UniqueTargets;
    if (bHit)
    {
        for (const FHitResult& Hit : Hits)
        {
            ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
            if (!TargetChampion || TargetChampion == this) continue;
            if (UniqueTargets.Contains(TargetChampion)) continue;
            UniqueTargets.Add(TargetChampion);
            Targets.Add(TargetChampion);
        }
    }

    ABaseChampion* NearestTarget = nullptr;
    float NearestDistSq = TNumericLimits<float>::Max();
    for (ABaseChampion* Target : Targets)
    {
        const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestTarget = Target;
        }
    }

    const float DamagePerTick = SkillComponent->GetQ_Data().BaseDamage[0] +
        StatComponent->GetStat().AttackDamage * 0.04f;

    for (ABaseChampion* TargetChampion : Targets)
    {
        float FinalDamage = DamagePerTick;
        if (TargetChampion == NearestTarget)
        {
            FinalDamage *= 1.25f;
        }

        UGameplayStatics::ApplyDamage(
            TargetChampion,
            FinalDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );

        int32& HitCount = E_HitCounts.FindOrAdd(TargetChampion);
        HitCount++;
        if (HitCount >= 6 && !E_ArmorReducedTargets.Contains(TargetChampion) && TargetChampion->StatComponent)
        {
            E_ArmorReducedTargets.Add(TargetChampion);
            const FChampionStat OriginalStat = TargetChampion->StatComponent->GetStat();
            FChampionStat ReducedStat = OriginalStat;
            ReducedStat.Armor *= 0.75f;
            TargetChampion->StatComponent->SetStat(ReducedStat);
            TWeakObjectPtr<ABaseChampion> TargetKey(TargetChampion);
            FTimerHandle ArmorReductionTimerHandle;
            GetWorldTimerManager().SetTimer(
                ArmorReductionTimerHandle,
                FTimerDelegate::CreateLambda([TargetKey, OriginalStat]()
                    {
                        if (TargetKey.IsValid() && TargetKey->StatComponent)
                        {
                            TargetKey->StatComponent->SetStat(OriginalStat);
                        }
                    }),
                6.0f,
                false
            );
        }
    }

    E_CurrentTick++;
    if (E_CurrentTick >= E_MaxTicks)
    {
        EndESpin();
    }
}

void AChampion_Garen::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack || bIsCastingR) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult HitResult;
    if (!PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult)) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(HitResult.GetActor());
    if (!IsValid(TargetChampion) || TargetChampion == this) return;

    const float Range = GetRSkillRange();
    const float Distance = GetDistanceTo(TargetChampion);

    if (Distance <= Range)
    {
        bIsChasingForR = false;
        ReservedRTarget = nullptr;
        Server_Skill_R(TargetChampion);
        return;
    }

    ReservedRTarget = TargetChampion;
    bIsChasingForR = true;

    if (ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->AddStatusTag(LOLTags::State_Moving);
        StateComp->RemoveStatusTag(LOLTags::State_Attacking);
    }
}

bool AChampion_Garen::Server_Skill_R_Validate(AActor* TargetActor)
{
    return true;
}

void AChampion_Garen::Server_Skill_R_Implementation(AActor* TargetActor)
{
    if (!SkillComponent || !StatComponent) return;
    if (!TargetActor) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
    if (!TargetChampion || TargetChampion == this) return;
    if (!TargetChampion->StatComponent) return;

    FSkillData& RData = SkillComponent->GetR_Data();
    const int32 SkillLevelIdx = 0;

    const float Range = RData.Range.IsValidIndex(SkillLevelIdx)
        ? RData.Range[SkillLevelIdx]
        : R_DefaultRange;

    if (GetDistanceTo(TargetChampion) > Range) return;

    if (!SkillComponent->TryCastSkill("R", 1)) return;

    bIsCastingR = true;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

    if (ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->RemoveStatusTag(LOLTags::State_Moving);
        StateComp->AddStatusTag(LOLTags::State_Attacking);
    }

    Multicast_PlayMontage(ChampionResource.RMontage[AM_SKIll_R_IDX], 1.0f);

    const float RLockDuration = 1.0f; // R 몽타주 길이에 맞게 조절
    GetWorldTimerManager().SetTimer(
        R_CastLockTimerHandle,
        this,
        &AChampion_Garen::EndRCastLock,
        RLockDuration,
        false
    );

    const float BaseDamage = RData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? RData.BaseDamage[SkillLevelIdx]
        : 0.0f;

    const float MaxHP = TargetChampion->StatComponent->GetStat().MaxHP;
    const float CurrentHP = TargetChampion->StatComponent->GetCurrentHP();
    const float MissingHP = FMath::Max(0.0f, MaxHP - CurrentHP);

    const float MissingHPRatio = RData.SecondaryValue.IsValidIndex(SkillLevelIdx)
        ? RData.SecondaryValue[SkillLevelIdx]
        : R_MissingHPRatio;

    const float SkillDamage = BaseDamage + MissingHP * MissingHPRatio;

    if (SkillDamage <= 0.0f) return;

    UGameplayStatics::ApplyDamage(
        TargetChampion,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamageTrueDamage::StaticClass()
    );
}

void AChampion_Garen::UpdateRChaseToCast()
{
    if (bIsStunned || bIsKnockedBack || !IsValid(ReservedRTarget))
    {
        bIsChasingForR = false;
        ReservedRTarget = nullptr;
        return;
    }

    ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>();
    ULOL_MoveComponent* MoveComp = FindComponentByClass<ULOL_MoveComponent>();
    if (!StateComp || !MoveComp) return;

    const float Range = GetRSkillRange();
    const float Distance = GetDistanceTo(ReservedRTarget);

    if (Distance <= Range)
    {
        ABaseChampion* Target = ReservedRTarget;

        bIsChasingForR = false;
        ReservedRTarget = nullptr;

        MoveComp->StopMovement();
        StateComp->RemoveStatusTag(LOLTags::State_Moving);

        Server_Skill_R(Target);
        return;
    }

    StateComp->AddStatusTag(LOLTags::State_Moving);
    StateComp->RemoveStatusTag(LOLTags::State_Attacking);

    MoveComp->TargetLocation = ReservedRTarget->GetActorLocation();

    FVector Direction = MoveComp->TargetLocation - GetActorLocation();
    Direction.Z = 0.f;

    AddMovementInput(Direction.GetSafeNormal(), 1.0f);
}

float AChampion_Garen::GetRSkillRange()
{
    if (!SkillComponent) return R_DefaultRange;

    FSkillData& RData = SkillComponent->GetR_Data();
    const int32 SkillLevelIdx = 0;

    return RData.Range.IsValidIndex(SkillLevelIdx)
        ? RData.Range[SkillLevelIdx]
        : R_DefaultRange;
}

void AChampion_Garen::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsLocallyControlled() && bIsChasingForR)
    {
        UpdateRChaseToCast();
    }
}

void AChampion_Garen::EndRCastLock()
{
    bIsCastingR = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    if (ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->RemoveStatusTag(LOLTags::State_Attacking);
    }
}