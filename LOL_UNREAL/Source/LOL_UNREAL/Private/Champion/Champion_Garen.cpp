#include "Champion/Champion_Garen.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Component/LOL_AttackComponent.h"
#include "Engine/StaticMesh.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Minion/BaseMinion.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Garen::AChampion_Garen()
{
    ChampionName = TEXT("Garen");
    SetChampionData(ChampionName);
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> REffectAsset(
        TEXT("/Game/Level/garen/garen_tex/ns_garen_r.ns_garen_r"));
    if (REffectAsset.Succeeded())
    {
        REffectSystem = REffectAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Garen R Niagara effect failed to load."));
    }

    WShieldComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GarenWShield"));
    WShieldComponent->SetupAttachment(RootComponent);
    WShieldComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WShieldComponent->SetGenerateOverlapEvents(false);
    WShieldComponent->SetCastShadow(false);
    WShieldComponent->SetHiddenInGame(true);
    WShieldComponent->SetVisibility(false, true);
    WShieldComponent->SetRelativeLocation(WShieldRelativeLocation);
    WShieldComponent->SetRelativeRotation(WShieldRelativeRotation);
    WShieldComponent->SetRelativeScale3D(WShieldRelativeScale);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> WShieldMeshAsset(
        TEXT("/Game/Level/garen/garen_tex/garen_w_shield.garen_w_shield"));
    if (WShieldMeshAsset.Succeeded())
    {
        WShieldComponent->SetStaticMesh(WShieldMeshAsset.Object);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Garen W shield mesh failed to load."));
    }
}

void AChampion_Garen::Multicast_SetWShieldVisible_Implementation(bool bVisible)
{
    if (!WShieldComponent) return;

    const FVector ShieldLocation = WShieldRelativeLocation.IsNearlyZero()
        ? FVector(0.0f, 0.0f, 80.0f)
        : WShieldRelativeLocation;
    const FVector ShieldScale = WShieldRelativeScale.GetMin() < 0.5f
        ? FVector(2.5f, 2.5f, 2.5f)
        : WShieldRelativeScale;

    WShieldComponent->SetRelativeLocation(ShieldLocation);
    WShieldComponent->SetRelativeRotation(WShieldRelativeRotation);
    WShieldComponent->SetRelativeScale3D(ShieldScale);
    WShieldComponent->SetVisibility(bVisible, true);
    WShieldComponent->SetHiddenInGame(!bVisible, true);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Garen W shield visible=%d Mesh=%s Location=%s Scale=%s"),
        bVisible ? 1 : 0,
        WShieldComponent->GetStaticMesh() ? *WShieldComponent->GetStaticMesh()->GetName() : TEXT("None"),
        *ShieldLocation.ToString(),
        *ShieldScale.ToString()
    );
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
    Multicast_SetWShieldVisible(true);

    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().SetTimer(W_BuffTimerHandle, this, &AChampion_Garen::EndWBuff, Duration, false);
}

void AChampion_Garen::EndWBuff()
{
    if (!bWActive || !StatComponent) return;

    bWActive = false;
    StatComponent->SetStat(W_OriginalStat);
    Multicast_SetWShieldVisible(false);
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
    E_MaxTicks = FMath::Max(E_TotalHits, 1);
    E_HitCounts.Empty();
    E_ArmorReducedTargets.Empty();

    const float TickInterval = FMath::Max(E_DefaultDuration, 0.1f) / static_cast<float>(E_MaxTicks);

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

    const float Radius = FMath::Max(E_DefaultRadius, 1.0f);
    const float RadiusSq = FMath::Square(Radius);

    TArray<AActor*> Targets;
    TSet<AActor*> UniqueTargets;

    auto AddTargetsInRange = [&](TSubclassOf<AActor> TargetClass)
    {
        TArray<AActor*> CandidateActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), TargetClass, CandidateActors);

        for (AActor* TargetActor : CandidateActors)
        {
            if (!IsValid(TargetActor) || TargetActor == this) continue;

            ULOL_StateComponent* TargetState = TargetActor->FindComponentByClass<ULOL_StateComponent>();
            if (!TargetState || TargetState->HasStatusTag(LOLTags::State_Dead)) continue;
            if (!IsEnemyActor(TargetActor)) continue;

            const float DistSq = FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation());
            if (DistSq > RadiusSq) continue;
            if (UniqueTargets.Contains(TargetActor)) continue;

            UniqueTargets.Add(TargetActor);
            Targets.Add(TargetActor);
        }
    };

    AddTargetsInRange(ABaseChampion::StaticClass());
    AddTargetsInRange(ABaseMinion::StaticClass());
    AddTargetsInRange(ABaseJungleMonster::StaticClass());
    AActor* NearestTarget = nullptr;
    float NearestDistSq = TNumericLimits<float>::Max();
    for (AActor* Target : Targets)
    {
        const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestTarget = Target;
        }
    }

    const float DamagePerTick = (EData.BaseDamage.IsValidIndex(0) ? EData.BaseDamage[0] : 0.0f) +
        StatComponent->GetStat().AttackDamage * E_ADRatio;

    for (AActor* TargetActor : Targets)
    {
        float FinalDamage = DamagePerTick;
        if (TargetActor == NearestTarget)
        {
            FinalDamage *= 1.25f;
        }

        UGameplayStatics::ApplyDamage(
            TargetActor,
            FinalDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );

        const TWeakObjectPtr<AActor> TargetKey(TargetActor);
        int32& HitCount = E_HitCounts.FindOrAdd(TargetKey);
        HitCount++;

        ULOL_StatComponent* TargetStat =
            TargetActor->FindComponentByClass<ULOL_StatComponent>();
        if (HitCount >= E_ArmorReductionHitCount && !E_ArmorReducedTargets.Contains(TargetKey) && TargetStat)
        {
            E_ArmorReducedTargets.Add(TargetKey);
            const FChampionStat OriginalStat = TargetStat->GetStat();
            FChampionStat ReducedStat = OriginalStat;
            ReducedStat.Armor *= (1.0f - FMath::Clamp(E_ArmorReductionRatio, 0.0f, 1.0f));
            TargetStat->SetStat(ReducedStat);

            FTimerHandle ArmorReductionTimerHandle;
            GetWorldTimerManager().SetTimer(
                ArmorReductionTimerHandle,
                FTimerDelegate::CreateLambda([TargetKey, OriginalStat]()
                    {
                        if (TargetKey.IsValid())
                        {
                            if (ULOL_StatComponent* RestoreStat =
                                TargetKey->FindComponentByClass<ULOL_StatComponent>())
                            {
                                RestoreStat->SetStat(OriginalStat);
                            }
                        }
                    }),
                E_ArmorReductionDuration,
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

    Multicast_SpawnREffect(TargetChampion->GetActorLocation());

    UGameplayStatics::ApplyDamage(
        TargetChampion,
        SkillDamage,
        GetController(),
        this,
        ULOL_DamageTrueDamage::StaticClass()
    );
}

void AChampion_Garen::Multicast_SpawnREffect_Implementation(FVector SpawnLocation)
{
    SpawnREffect(SpawnLocation);
}

void AChampion_Garen::SpawnREffect(FVector SpawnLocation)
{
    if (!GetWorld()) return;

    if (!REffectSystem)
    {
        REffectSystem = LoadObject<UNiagaraSystem>(
            nullptr,
            TEXT("/Game/Level/garen/garen_tex/ns_garen_r.ns_garen_r")
        );
    }

    if (!REffectSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("Garen R Niagara effect skipped: REffectSystem is null."));
        return;
    }

    const FVector FinalLocation = SpawnLocation + REffectLocationOffset;
    UNiagaraComponent* EffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        REffectSystem,
        FinalLocation,
        REffectRotation,
        REffectScale,
        true,
        true,
        ENCPoolMethod::None,
        true
    );

    if (!EffectComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Garen R Niagara effect spawn failed."));
        return;
    }

    const TWeakObjectPtr<UNiagaraComponent> EffectWeak(EffectComponent);
    FTimerHandle DestroyEffectTimerHandle;
    GetWorldTimerManager().SetTimer(
        DestroyEffectTimerHandle,
        FTimerDelegate::CreateLambda([EffectWeak]()
        {
            if (EffectWeak.IsValid())
            {
                EffectWeak->DestroyComponent();
            }
        }),
        FMath::Max(REffectLifeTime, 0.05f),
        false
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Garen R Niagara effect spawned. Location=%s Scale=%s LifeTime=%.2f"),
        *FinalLocation.ToString(),
        *REffectScale.ToString(),
        REffectLifeTime
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

bool AChampion_Garen::IsMoveInputBlocked() const
{
    return bIsCastingR;
}
void AChampion_Garen::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsSpinning && ChampionResource.EMontage.IsValidIndex(AM_SKIll_E_IDX))
    {
        UAnimMontage* EMontage = ChampionResource.EMontage[AM_SKIll_E_IDX];
        UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
        if (EMontage && AnimInst && !AnimInst->Montage_IsPlaying(EMontage))
        {
            AnimInst->Montage_Play(EMontage, 1.0f);
        }
    }

    if (IsLocallyControlled() && bIsChasingForR)
    {
        UpdateRChaseToCast();
    }
}

void AChampion_Garen::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChampion_Garen, bIsSpinning);
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