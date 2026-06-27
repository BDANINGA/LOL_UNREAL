#include "Champion/Champion_Jax.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
#include "UObject/ConstructorHelpers.h"


AChampion_Jax::AChampion_Jax()
{
    ChampionName = TEXT("Jax");
    SetChampionData(ChampionName);

    static ConstructorHelpers::FObjectFinder<UAnimSequence> FallbackAnimationAsset(
        TEXT("/Game/Level/jax/jax_as/Unreal_jax_ani_run_attack_1_Anim.Unreal_jax_ani_run_attack_1_Anim"));
    if (FallbackAnimationAsset.Succeeded())
    {
        FallbackSkillAnimation = FallbackAnimationAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> RIdleMontageAsset(
        TEXT("/Game/Level/jax/jax_am/am_spell4_idle_Montage.am_spell4_idle_Montage"));
    if (RIdleMontageAsset.Succeeded())
    {
        GrandmastersMightIdleMontage = RIdleMontageAsset.Object;
    }

    GetMesh()->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));
}

int32 JaxSkillLevelIndex = 0;

float GetSkillValue(
    const TArray<float>& Values,
    int32 Index,
    float Fallback)
{
    return Values.IsValidIndex(Index)
        ? Values[Index]
        : Fallback;
}

void AChampion_Jax::Skill_Q()
{
    if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) return;

    FHitResult Hit;
    if (!PlayerController->GetHitResultUnderCursor(ECC_Pawn, false, Hit)) return;

    ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
    if (!IsValidLeapStrikeTarget(Target)) return;

    if (GetDistanceTo(Target) <= GetQSkillRange())
    {
        bIsChasingForQ = false;
        ReservedQTarget = nullptr;
        Server_Skill_Q(Target);
        return;
    }

    ReservedQTarget = Target;
    bIsChasingForQ = true;
}

void AChampion_Jax::Skill_W()
{
    if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
    Server_Skill_W();
}

void AChampion_Jax::Skill_E()
{
    if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
    Server_Skill_E();
}

void AChampion_Jax::Skill_R()
{
    if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
    Server_Skill_R();
}

bool AChampion_Jax::Server_Skill_Q_Validate(ACharacter* Target)
{
    return IsValidLeapStrikeTarget(Target);
}

void AChampion_Jax::Server_Skill_Q_Implementation(ACharacter* Target)
{
    if (!SkillComponent || !StatComponent || !IsValidLeapStrikeTarget(Target)) return;
    if (bIsLeaping || GetDistanceTo(Target) > GetQSkillRange() + 50.0f) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FVector Direction = Target->GetActorLocation() - GetActorLocation();
    Direction.Z = 0.0f;
    const FRotator FacingRotation = Direction.IsNearlyZero()
        ? GetActorRotation()
        : Direction.Rotation();

    if (AttackComponent)
    {
        AttackComponent->CancelAttack();
        AttackComponent->CombatTarget = nullptr;
        AttackComponent->HitTarget = nullptr;
    }

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }

    Multicast_PlayJaxSkillAnimation(0, 1.0f, FacingRotation);

    LeapStart = GetActorLocation();
    LeapTarget = Target;
    LeapElapsed = 0.0f;
    bIsLeaping = true;
}

bool AChampion_Jax::Server_Skill_W_Validate()
{
    return true;
}

void AChampion_Jax::Server_Skill_W_Implementation()
{
    if (!SkillComponent || !AttackComponent) return;
    if (!SkillComponent->TryCastSkill("W", 1)) return;

    bWEmpowered = true;

    // Empower resets Jax's basic attack timer.
    GetWorldTimerManager().ClearTimer(AttackComponent->AttackTimerHandle);
    AttackComponent->ResetAttack();

    GetWorldTimerManager().ClearTimer(WTimerHandle);
    GetWorldTimerManager().SetTimer(
        WTimerHandle,
        this,
        &AChampion_Jax::EndWEmpower,
        WEmpowerDuration,
        false
    );
}

bool AChampion_Jax::Server_Skill_E_Validate()
{
    return true;
}

void AChampion_Jax::Server_Skill_E_Implementation()
{
    if (!SkillComponent) return;

    if (bCounterStrikeActive) return;

    if (!SkillComponent->TryCastSkill("E", 1)) return;

    bCounterStrikeActive = true;
    CounterStrikeDodgedAttacks = 0;
    Multicast_PlayJaxSkillAnimation(2, 0.9f, GetActorRotation());

    const FSkillData& EData = SkillComponent->GetE_Data();
    const float Duration = GetSkillValue(EData.Duration, JaxSkillLevelIndex, 2.0f);

    GetWorldTimerManager().ClearTimer(ETimerHandle);
    GetWorldTimerManager().SetTimer(
        ETimerHandle,
        this,
        &AChampion_Jax::FinishCounterStrike,
        Duration > 0.0f ? Duration : 2.0f,
        false
    );
}

bool AChampion_Jax::Server_Skill_R_Validate()
{
    return true;
}

void AChampion_Jax::Server_Skill_R_Implementation()
{
    if (!SkillComponent || !StatComponent || bGrandmastersMightActive) return;
    if (!SkillComponent->TryCastSkill("R", 2)) return;

    const FChampionStat CurrentStat = StatComponent->GetStat();
    RArmorBonusApplied = RBaseArmorBonus +
        CurrentStat.BonusAttackDamage * RBonusADToArmorRatio;
    RSpellBlockBonusApplied = RBaseSpellBlockBonus +
        CurrentStat.AbilityPower * RAPToSpellBlockRatio;

    FChampionStat BuffedStat = CurrentStat;
    BuffedStat.Armor += RArmorBonusApplied;
    BuffedStat.SpellBlock += RSpellBlockBonusApplied;
    StatComponent->SetStat(BuffedStat);

    bGrandmastersMightActive = true;
    Multicast_PlayJaxSkillAnimation(3, 0.8f, GetActorRotation());
    Multicast_SetGrandmastersMightIdle(true);

    const FSkillData& RData = SkillComponent->GetR_Data();
    const float Duration = GetSkillValue(RData.Duration, JaxSkillLevelIndex, 8.0f);
    const float CastTime = FMath::Max(0.0f, RData.CastTime);

    GetWorldTimerManager().ClearTimer(RDamageTimerHandle);
    if (CastTime > 0.0f)
    {
        GetWorldTimerManager().SetTimer(
            RDamageTimerHandle,
            this,
            &AChampion_Jax::ApplyGrandmastersMightActiveDamage,
            CastTime,
            false
        );
    }
    else
    {
        ApplyGrandmastersMightActiveDamage();
    }

    GetWorldTimerManager().ClearTimer(UltTimerHandle);
    GetWorldTimerManager().SetTimer(
        UltTimerHandle,
        this,
        &AChampion_Jax::EndGrandmastersMight,
        Duration > 0.0f ? Duration : 8.0f,
        false
    );
}

void AChampion_Jax::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsLocallyControlled() && bIsChasingForQ)
    {
        UpdateQChaseToCast();
    }

    if (HasAuthority() && bIsLeaping)
    {
        UpdateLeap(DeltaTime);
    }

    UpdateGrandmastersMightIdleAnimation();
}

void AChampion_Jax::UpdateGrandmastersMightIdleAnimation()
{
    if (!bGrandmastersMightIdleEnabled || !GrandmastersMightIdleMontage || !GetMesh()) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;

    const bool bIsMoving = GetVelocity().SizeSquared2D() > FMath::Square(5.0f);
    const bool bIsPlayingRIdle = AnimInstance->Montage_IsPlaying(GrandmastersMightIdleMontage);

    if (bIsMoving)
    {
        if (bIsPlayingRIdle)
        {
            AnimInstance->Montage_Stop(0.1f, GrandmastersMightIdleMontage);
        }
        return;
    }

    // Wait until cast/attack/skill montages finish, then switch to the R idle loop.
    if (!bIsPlayingRIdle && !AnimInstance->IsAnyMontagePlaying())
    {
        AnimInstance->Montage_Play(GrandmastersMightIdleMontage, 1.0f);

        if (GrandmastersMightIdleMontage->GetNumSections() > 0)
        {
            const FName IdleSection = GrandmastersMightIdleMontage->GetSectionName(0);
            AnimInstance->Montage_SetNextSection(
                IdleSection,
                IdleSection,
                GrandmastersMightIdleMontage
            );
        }
    }
}

void AChampion_Jax::UpdateQChaseToCast()
{
    if (bIsStunned || bIsKnockedBack || !IsValid(ReservedQTarget))
    {
        bIsChasingForQ = false;
        ReservedQTarget = nullptr;
        return;
    }

    if (GetDistanceTo(ReservedQTarget) <= GetQSkillRange())
    {
        ACharacter* Target = ReservedQTarget;
        bIsChasingForQ = false;
        ReservedQTarget = nullptr;

        if (MoveComponent)
        {
            MoveComponent->StopMovement();
        }
        Server_Skill_Q(Target);
        return;
    }

    if (MoveComponent)
    {
        MoveComponent->TargetLocation = ReservedQTarget->GetActorLocation();
    }

    FVector Direction = ReservedQTarget->GetActorLocation() - GetActorLocation();
    Direction.Z = 0.0f;
    if (!Direction.IsNearlyZero())
    {
        AddMovementInput(Direction.GetSafeNormal(), 1.0f);
    }
}

void AChampion_Jax::UpdateLeap(float DeltaTime)
{
    if (!IsValid(LeapTarget))
    {
        FinishLeap();
        return;
    }

    LeapElapsed += DeltaTime;
    const float Duration = FMath::Max(QLeapDuration, KINDA_SMALL_NUMBER);
    const float Alpha = FMath::Clamp(LeapElapsed / Duration, 0.0f, 1.0f);

    FVector Direction = LeapTarget->GetActorLocation() - LeapStart;
    Direction.Z = 0.0f;

    const float TargetOffset = GetCapsuleComponent()->GetScaledCapsuleRadius() +
        LeapTarget->GetCapsuleComponent()->GetScaledCapsuleRadius();
    FVector LandingLocation = LeapTarget->GetActorLocation() -
        Direction.GetSafeNormal() * TargetOffset;
    LandingLocation.Z = LeapStart.Z;

    SetActorLocation(
        FMath::Lerp(LeapStart, LandingLocation, Alpha),
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    if (Alpha >= 1.0f)
    {
        FinishLeap();
    }
}

void AChampion_Jax::FinishLeap()
{
    if (!bIsLeaping) return;

    ACharacter* Target = LeapTarget;
    bIsLeaping = false;
    LeapTarget = nullptr;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    if (!IsValid(Target) || !SkillComponent || !StatComponent) return;
    if (!IsEnemyActor(Target)) return;

    const FSkillData& QData = SkillComponent->GetQ_Data();
    const float BaseDamage = GetSkillValue(QData.BaseDamage, JaxSkillLevelIndex, 65.0f);
    const float APRatio = GetSkillValue(QData.SecondaryValue, JaxSkillLevelIndex, 0.6f);
    const FChampionStat Stat = StatComponent->GetStat();
    // Q = base damage + 100% bonus AD + data-driven AP ratio.
    const float SkillDamage = BaseDamage +
        Stat.BonusAttackDamage * QBonusADRatio +
        Stat.AbilityPower * APRatio;

    UGameplayStatics::ApplyDamage(
        Target,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamagePhysical::StaticClass()
    );

    if (bWEmpowered)
    {
        ApplyEmpowerDamage(Target);
    }
}

void AChampion_Jax::OnBasicAttackHit(ACharacter* Target)
{
    if (!HasAuthority() || !IsValid(Target) || Target == this) return;

    if (bWEmpowered)
    {
        ApplyEmpowerDamage(Target);
    }

    ApplyGrandmastersMightPassive(Target);
}

void AChampion_Jax::ApplyEmpowerDamage(ACharacter* Target)
{
    if (!bWEmpowered || !IsValid(Target) || !SkillComponent || !StatComponent) return;

    const FSkillData& WData = SkillComponent->GetW_Data();
    const float BaseDamage = GetSkillValue(WData.BaseDamage, JaxSkillLevelIndex, 50.0f);
    const float APRatio = GetSkillValue(WData.SecondaryValue, JaxSkillLevelIndex, 0.6f);
    // W = base damage + data-driven AP ratio.
    const float SkillDamage = BaseDamage +
        StatComponent->GetStat().AbilityPower * APRatio;

    UGameplayStatics::ApplyDamage(
        Target,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamageMagic::StaticClass()
    );

    bWEmpowered = false;
    GetWorldTimerManager().ClearTimer(WTimerHandle);
}

void AChampion_Jax::ApplyGrandmastersMightPassive(ACharacter* Target)
{
    if (!IsValid(Target) || !SkillComponent || !StatComponent) return;

    GrandmastersMightHitCount++;
    if (GrandmastersMightHitCount < FMath::Max(1, RPassiveHitThreshold)) return;

    GrandmastersMightHitCount = 0;

    const FSkillData& RData = SkillComponent->GetR_Data();
    const float BaseDamage = GetSkillValue(RData.BaseDamage, JaxSkillLevelIndex, 60.0f);
    const float APRatio = GetSkillValue(RData.SecondaryValue, JaxSkillLevelIndex, 0.6f);
    // Every third basic attack: R base damage + data-driven AP ratio.
    const float SkillDamage = BaseDamage +
        StatComponent->GetStat().AbilityPower * APRatio;

    UGameplayStatics::ApplyDamage(
        Target,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamageMagic::StaticClass()
    );
}

void AChampion_Jax::ApplyGrandmastersMightActiveDamage()
{
    if (!HasAuthority() || !SkillComponent || !StatComponent) return;

    const FSkillData& RData = SkillComponent->GetR_Data();
    const float Radius = GetSkillValue(RData.Range, JaxSkillLevelIndex, 375.0f);
    const float BaseDamage = GetSkillValue(RData.BaseDamage, JaxSkillLevelIndex, 60.0f);
    const float APRatio = GetSkillValue(RData.SecondaryValue, JaxSkillLevelIndex, 0.6f);
    const float SkillDamage = BaseDamage +
        StatComponent->GetStat().AbilityPower * APRatio;

    TArray<FHitResult> Hits;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(Radius),
        QueryParams
    );

    if (!bHit) return;

    TSet<TWeakObjectPtr<AActor>> DamagedTargets;
    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!IsValid(Target) || Target == this || DamagedTargets.Contains(Target)) continue;
        if (!Target->FindComponentByClass<ULOL_StateComponent>() || !IsEnemyActor(Target)) continue;

        DamagedTargets.Add(Target);
        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}

float AChampion_Jax::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    const bool bIsBasicAttack =
        Cast<ABaseChampion>(DamageCauser) != nullptr &&
        DamageEvent.DamageTypeClass == UDamageType::StaticClass();

    if (HasAuthority() && bCounterStrikeActive && bIsBasicAttack)
    {
        CounterStrikeDodgedAttacks++;
        return 0.0f;
    }

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AChampion_Jax::FinishCounterStrike()
{
    if (!HasAuthority() || !bCounterStrikeActive || !SkillComponent || !StatComponent) return;

    bCounterStrikeActive = false;
    GetWorldTimerManager().ClearTimer(ETimerHandle);
    Multicast_PlayJaxSkillAnimation(2, 1.2f, GetActorRotation());

    const FSkillData& EData = SkillComponent->GetE_Data();
    const float Radius = GetSkillValue(EData.Range, JaxSkillLevelIndex, 300.0f);
    const float BaseDamage = GetSkillValue(EData.BaseDamage, JaxSkillLevelIndex, 55.0f);
    const int32 DamageStacks = FMath::Min(CounterStrikeDodgedAttacks, EMaxDamageStacks);
    const float DodgeMultiplier = 1.0f + DamageStacks * EDodgedAttackDamageRatio;

    TArray<FHitResult> Hits;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(Radius),
        QueryParams
    );

    TSet<TWeakObjectPtr<AActor>> DamagedTargets;
    if (bHit)
    {
        for (const FHitResult& Hit : Hits)
        {
            AActor* Target = Hit.GetActor();
            if (!IsValid(Target) || Target == this || DamagedTargets.Contains(Target)) continue;
            if (!Target->FindComponentByClass<ULOL_StateComponent>() || !IsEnemyActor(Target)) continue;
            DamagedTargets.Add(Target);

            ULOL_StatComponent* TargetStat = Target->FindComponentByClass<ULOL_StatComponent>();
            const float TargetMaxHP = TargetStat
                ? TargetStat->GetStat().MaxHP
                : 0.0f;
            // E = (base + 70% AP + 4% target max HP) * dodged-attack multiplier.
            const float SkillDamage = (
                BaseDamage +
                StatComponent->GetStat().AbilityPower * EAbilityPowerRatio +
                TargetMaxHP * ETargetMaxHPRatio
            ) * DodgeMultiplier;

            UGameplayStatics::ApplyDamage(
                Target,
                SkillDamage,
                GetController(),
                this,
                ULOL_DamageMagic::StaticClass()
            );
            if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
            {
                TargetChampion->Multicast_ApplyStun(EStunDuration);
            }
        }
    }

    CounterStrikeDodgedAttacks = 0;
}

void AChampion_Jax::EndWEmpower()
{
    bWEmpowered = false;
}

void AChampion_Jax::EndGrandmastersMight()
{
    if (!bGrandmastersMightActive || !StatComponent) return;

    Multicast_SetGrandmastersMightIdle(false);

    FChampionStat CurrentStat = StatComponent->GetStat();
    CurrentStat.Armor = FMath::Max(0.0f, CurrentStat.Armor - RArmorBonusApplied);
    CurrentStat.SpellBlock = FMath::Max(0.0f, CurrentStat.SpellBlock - RSpellBlockBonusApplied);
    StatComponent->SetStat(CurrentStat);

    bGrandmastersMightActive = false;
    RArmorBonusApplied = 0.0f;
    RSpellBlockBonusApplied = 0.0f;
}

void AChampion_Jax::Multicast_SetGrandmastersMightIdle_Implementation(bool bEnabled)
{
    bGrandmastersMightIdleEnabled = bEnabled;

    if (!bEnabled && GrandmastersMightIdleMontage && GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->Montage_Stop(0.15f, GrandmastersMightIdleMontage);
        }
    }
}

float AChampion_Jax::GetQSkillRange() const
{
    if (!SkillComponent) return 600.0f;
    return GetSkillValue(SkillComponent->GetQ_Data().Range, JaxSkillLevelIndex, 600.0f);
}

bool AChampion_Jax::IsValidLeapStrikeTarget(AActor* Target) const
{
    if (!IsValid(Target) || Target == this)
    {
        return false;
    }

    if (Cast<ABaseMinion>(Target) || Cast<ABaseJungleMonster>(Target))
    {
        return true;
    }

    if (Cast<ABaseChampion>(Target))
    {
        return IsEnemyActor(Target);
    }

    return false;
}

UAnimMontage* AChampion_Jax::GetSkillMontage(uint8 SkillIndex) const
{
    const TArray<UAnimMontage*>* Montages = nullptr;
    int32 MontageIndex = 0;
    switch (SkillIndex)
    {
    case 0:
        Montages = &ChampionResource.QMontage;
        MontageIndex = AM_SKIll_Q_IDX;
        break;
    case 1:
        Montages = &ChampionResource.WMontage;
        MontageIndex = AM_SKIll_W_IDX;
        break;
    case 2:
        Montages = &ChampionResource.EMontage;
        MontageIndex = AM_SKIll_E_IDX;
        break;
    case 3:
        Montages = &ChampionResource.RMontage;
        MontageIndex = AM_SKIll_R_IDX;
        break;
    default: break;
    }

    return Montages && Montages->IsValidIndex(MontageIndex)
        ? (*Montages)[MontageIndex]
        : nullptr;
}

UAnimMontage* AChampion_Jax::GetWEmpoweredAttackMontage() const
{
    return GetSkillMontage(1);
}

void AChampion_Jax::Multicast_PlayJaxSkillAnimation_Implementation(
    uint8 SkillIndex,
    float PlayRate,
    FRotator FacingRotation)
{
    FacingRotation.Pitch = 0.0f;
    FacingRotation.Roll = 0.0f;
    SetActorRotation(FacingRotation);

    if (UAnimMontage* Montage = GetSkillMontage(SkillIndex))
    {
        PlayAnimMontage(Montage, PlayRate);
        return;
    }

    if (FallbackSkillAnimation && GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            AnimInstance->PlaySlotAnimationAsDynamicMontage(
                FallbackSkillAnimation,
                TEXT("DefaultSlot"),
                0.05f,
                0.1f,
                PlayRate,
                1
            );
        }
    }
}
