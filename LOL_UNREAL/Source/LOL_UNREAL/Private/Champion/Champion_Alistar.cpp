#include "Champion/Champion_Alistar.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"


AChampion_Alistar::AChampion_Alistar()
{
    ChampionName = TEXT("Alistar");
    SetChampionData(ChampionName);

    RShieldComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AlistarRShield"));
    RShieldComponent->SetupAttachment(RootComponent);
    RShieldComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RShieldComponent->SetGenerateOverlapEvents(false);
    RShieldComponent->SetCastShadow(false);
    RShieldComponent->SetHiddenInGame(true);
    RShieldComponent->SetVisibility(false, true);
    RShieldComponent->SetRelativeLocation(RShieldRelativeLocation);
    RShieldComponent->SetRelativeRotation(RShieldRelativeRotation);
    RShieldComponent->SetRelativeScale3D(RShieldRelativeScale);

    UStaticMesh* ShieldMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Level/alistar/alistar_tex/alistar_base_r_shield/StaticMeshes/alistar_base_r_shield.alistar_base_r_shield")
    );
    if (ShieldMesh)
    {
        RShieldComponent->SetStaticMesh(ShieldMesh);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Alistar R shield mesh failed to load."));
    }
}

void AChampion_Alistar::Multicast_PlayRMontage_Implementation()
{
    if (ChampionResource.RMontage[AM_SKIll_R_IDX])
    {
        PlayAnimMontage(ChampionResource.RMontage[AM_SKIll_R_IDX], 1.0f);
    }
}

void AChampion_Alistar::Multicast_SetRShieldVisible_Implementation(bool bVisible)
{
    if (!RShieldComponent) return;

    RShieldComponent->SetRelativeLocation(RShieldRelativeLocation);
    RShieldComponent->SetRelativeRotation(RShieldRelativeRotation);
    RShieldComponent->SetRelativeScale3D(RShieldRelativeScale);
    RShieldComponent->SetVisibility(bVisible, true);
    RShieldComponent->SetHiddenInGame(!bVisible, true);
}

void AChampion_Alistar::EndQCast()
{
    // ?´ë™ ë³µêµ¬
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    bCanAttack = true;
}
void AChampion_Alistar::Skill_Q()
{
    if (!IsLocallyControlled()) return;
   
    // ë³¸ì¸ ?œì „ ? ê¸ˆ (ë³¸ì¸ ?´ë¼ ì¦‰ì‹œ ë°˜ì‘)
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // ë³¸ì¸ ?œì „ ???€ ?€?´ë¨¸
    GetWorld()->GetTimerManager().SetTimer(
        Q_CastTimerHandle,
        this,
        &AChampion_Alistar::EndQCast,
        Q_CastTime,
        false
    );

    // ??ì§„ì§œ ë¡œì§?€ ?œë²„?ì„œ
    Server_Skill_Q();
}

bool AChampion_Alistar::Server_Skill_Q_Validate() { return true; }

void AChampion_Alistar::Server_Skill_Q_Implementation()
{
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    // ???œê° ?¨ê³¼???œë²„?ì„œ ë©€?°ìº?¤íŠ¸ (ëª¨ë“  ?´ë¼???„íŒŒ)
    Multicast_PlayMontage(ChampionResource.QMontage[AM_SKIll_Q_IDX], 2.0f);

    // ë²”ìœ„ ????ê²€ì¶?
    FVector Center = GetActorLocation();
    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(250.0f);

    bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        Center,
        Center,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (!bHit) return;

    // ê°??ì—ê²?ì²˜ë¦¬ ?ìš©
    for (auto& Hit : Hits)
    {
        ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
        if (!Target || !IsEnemyActor(Target)) continue;

        // ??ëª¨ì…˜Â·?´ë™ ?•ë¦¬
        Target->StopAnimMontage();
        UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement();
        if (MoveComp)
        {
            MoveComp->StopMovementImmediately();
            MoveComp->CurrentRootMotion.Clear();
            MoveComp->SetMovementMode(MOVE_Falling);
        }

        // ìº¡ìŠ ì¶©ëŒ ë¬´ì‹œ
        Target->MoveIgnoreActorAdd(this);

        // ?ì–´ë³?ë°œì‚¬
        Target->LaunchCharacter(FVector(0, 0, 600.0f), true, true);

        // ?¤í„´ + ?ì–´ë³??íƒœ ì²˜ë¦¬
        if (ABaseChampion* Champ = Cast<ABaseChampion>(Target))
        {
            Champ->SetIsKnockedBack(true);
            Champ->ApplyStun(2.0f);

            // 1ì´????íƒœ ë³µêµ¬
            FTimerHandle AirTimer;
            GetWorld()->GetTimerManager().SetTimer(AirTimer, [Champ, this]() {
                if (IsValid(Champ))
                {
                    Champ->SetIsKnockedBack(false);
                    if (IsValid(this))
                    {
                        Champ->MoveIgnoreActorRemove(this);
                    }
                }
                }, 1.0f, false);
        }

        // ?°ë?ì§€ ?ìš©
        float SkillDamage = SkillComponent->GetQ_Data().BaseDamage[0] +
            StatComponent->GetStat().AttackDamage * 0.8f;

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}


void AChampion_Alistar::Skill_W()
{
    if (!IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult HitResult;
    if (!PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult)) return;

    ACharacter* Target = Cast<ACharacter>(HitResult.GetActor());
    if (!IsValid(Target) || !IsEnemyActor(Target)) return;

    const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

    if (Distance <= W_CastRange)
    {
        bIsChasingForW = false;
        ReservedWTarget = nullptr;

        Server_Skill_W(Target);
        return;
    }

    ReservedWTarget = Target;
    bIsChasingForW = true;

    if (ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->AddStatusTag(LOLTags::State_Moving);
        StateComp->RemoveStatusTag(LOLTags::State_Attacking);
    }
}

bool AChampion_Alistar::Server_Skill_W_Validate(ACharacter* Target)
{
    if (!IsValid(Target)) return false;
    return true;
}

void AChampion_Alistar::Server_Skill_W_Implementation(ACharacter* Target)
{
    if (!SkillComponent) return;
    if (!IsValid(Target) || !IsEnemyActor(Target)) return;

    const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    if (Distance > W_CastRange + 50.0f) return;

    if (!SkillComponent->TryCastSkill("W", 1)) return;

    CurrentWTarget = Target;
    bIsW_Dashing = true;

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        WDashPreviousCollisionEnabled = Capsule->GetCollisionEnabled();
        WDashPreviousPawnResponse =
            Capsule->GetCollisionResponseToChannel(ECC_Pawn);
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    FVector DashDirection = Target->GetActorLocation() - GetActorLocation();
    DashDirection.Z = 0.0f;

    FRotator LookRotation = DashDirection.Rotation();
    LookRotation.Pitch = 0.0f;
    LookRotation.Roll = 0.0f;

    Multicast_SetTargetAndPlayMontage(
        ChampionResource.WMontage[AM_SKIll_W_IDX],
        2.0f,
        LookRotation
    );
}

void AChampion_Alistar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsLocallyControlled() && bIsChasingForW)
    {
        UpdateWChaseToCast();
    }

    if (!HasAuthority()) return;

    if (bIsKnockedBack)
    {
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->StopMovementImmediately();
        }
        return;
    }

    if (bIsW_Dashing && !IsValid(CurrentWTarget))
    {
        RestoreWCollision();
        bIsW_Dashing = false;
        CurrentWTarget = nullptr;
        return;
    }

    if (bIsW_Dashing && CurrentWTarget)
    {
        FVector CurrentLoc = GetActorLocation();
        FVector TargetLoc = CurrentWTarget->GetActorLocation();
        float Distance = FVector::Dist(CurrentLoc, TargetLoc);

        FVector NewLocation = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, 8.0f);
        SetActorLocation(NewLocation, false);

        if (Distance < 100.0f)
        {
            ApplyWKnockback(CurrentWTarget);
        }
    }
}

void AChampion_Alistar::UpdateWChaseToCast()
{
    if (!IsValid(ReservedWTarget))
    {
        bIsChasingForW = false;
        ReservedWTarget = nullptr;
        return;
    }

    if (!IsEnemyActor(ReservedWTarget))
    {
        bIsChasingForW = false;
        ReservedWTarget = nullptr;
        return;
    }

    ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>();
    ULOL_MoveComponent* MoveComp = FindComponentByClass<ULOL_MoveComponent>();

    if (!StateComp || !MoveComp) return;

    if (StateComp->HasStatusTag(LOLTags::State_Dead)) return;

    const float Distance = FVector::Dist(GetActorLocation(), ReservedWTarget->GetActorLocation());

    if (Distance <= W_CastRange)
    {
        ACharacter* Target = ReservedWTarget;

        bIsChasingForW = false;
        ReservedWTarget = nullptr;

        MoveComp->StopMovement();
        StateComp->RemoveStatusTag(LOLTags::State_Moving);

        Server_Skill_W(Target);
        return;
    }

    StateComp->AddStatusTag(LOLTags::State_Moving);
    StateComp->RemoveStatusTag(LOLTags::State_Attacking);

    MoveComp->TargetLocation = ReservedWTarget->GetActorLocation();

    FVector Direction = MoveComp->TargetLocation - GetActorLocation();
    Direction.Z = 0.f;

    AddMovementInput(Direction.GetSafeNormal(), 1.0f);
}

void AChampion_Alistar::ApplyWKnockback(ACharacter* Target)
{
    if (!Target || !IsEnemyActor(Target))
    {
        RestoreWCollision();
        bIsW_Dashing = false;
        CurrentWTarget = nullptr;
        return;
    }

    // 1. ê³µí†µ ë¶€ëª??´ë˜?¤ì¸ ABaseChampion?¼ë¡œ ìºìŠ¤??
    ABaseChampion* Enemy = Cast<ABaseChampion>(Target);

    if (Enemy)
    {
        
        // [ì¤‘ìš”] 2. ?ë?ë°©ì˜ ?ìœ¨ ?´ë™ ë°?ê³µê²© ë¡œì§ ?¼ì‹œ ì¤‘ë‹¨
        // BaseChampion??Tick(CheckAttackRange)?ì„œ ??ë³€?˜ë? ì²´í¬?˜ì—¬ 
        // StopMovementImmediately()ê°€ ?¸ì¶œ?˜ëŠ” ê²ƒì„ ë§‰ì•„???©ë‹ˆ??
        Enemy->SetIsKnockedBack(true);

        // [ì¤‘ìš”] 3. ê³µê²© ? ë‹ˆë©”ì´??ê°•ì œ ì¤‘ë‹¨ (ë£¨íŠ¸ ëª¨ì…˜ ?´ì œ)
        // ?‰í? ì¤‘ì¼ ??ë£¨íŠ¸ ëª¨ì…˜???„ì¹˜ë¥?ê³ ì •?˜ëŠ” ê²ƒì„ ë°©ì??©ë‹ˆ??
        Enemy->StopAnimMontage();

        UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();
        if (MoveComp)
        {
            // ê¸°ì¡´??ëª¨ë“  ?ë„?€ ë£¨íŠ¸ ëª¨ì…˜ ?”ì—¬ ?°ì´?°ë? ?œê±°
            MoveComp->StopMovementImmediately();
            MoveComp->CurrentRootMotion.Clear();

            // ì§€ë©?ë§ˆì°°?¥ì„ ë¬´ì‹œ?˜ê¸° ?„í•´ ê³µì¤‘ ?íƒœë¡?ë³€ê²?
            MoveComp->SetMovementMode(MOVE_Falling);
        }

        // 4. ?‰ë°± ë°©í–¥ ë°??ë„ ê³„ì‚°
        FVector AlistarLoc = GetActorLocation();
        FVector EnemyLoc = Enemy->GetActorLocation();

        // ë°©í–¥?€ ?˜í‰(XY)?¼ë¡œë§?ê³„ì‚°
        FVector PushDir = (EnemyLoc - AlistarLoc).GetSafeNormal2D();

        // 0.5ì´??™ì•ˆ ??600~700 ? ë‹›??? ë ¤ë³´ë‚¼ ?ë„ ?¤ì •
        FVector LaunchVel = PushDir * 1200.0f;
        LaunchVel.Z = 100.0f; // ?ì–´ë³??¨ê³¼

        // 5. ?ë?ë°?? ë¦¬ê¸?(ê¸°ì¡´ ?ë„ ë¬´ì‹œ ?µì…˜ true, true)
        Enemy->LaunchCharacter(LaunchVel, true, true);

        // 6. 0.5ì´??‰ë°± ?œê°„) ?„ì— ?ë?ë°©ì˜ ?íƒœ(bIsKnockedBack)ë¥??•ìƒ?¼ë¡œ ë³µêµ¬
        // ?Œë‹¤ ?¨ìˆ˜ ?´ì—??IsValid ì²´í¬ë¥??µí•´ ìºë¦­?°ê? ?Œê´´?˜ì? ?Šì•˜?”ì? ?•ì¸?©ë‹ˆ??
        FTimerHandle RecoveryTimer;
        GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, FTimerDelegate::CreateLambda([Enemy]() {
            if (IsValid(Enemy))
            {
                Enemy->SetIsKnockedBack(false);
            }
            }), 0.5f, false);
    }

    // 7. ?Œë¦¬?¤í? ë³¸ì¸ ì²˜ë¦¬ (?€ê²Ÿì´ ?ˆë˜ ?„ì¹˜??ë©ˆì¶¤)
    SetActorLocation(Target->GetActorLocation(), false);
    GetCharacterMovement()->StopMovementImmediately();

    // ?íƒœ ë³€??ì´ˆê¸°??
    RestoreWCollision();
    bIsW_Dashing = false;
    CurrentWTarget = nullptr;
}

void AChampion_Alistar::RestoreWCollision()
{
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(
            ECC_Pawn,
            WDashPreviousPawnResponse
        );
        Capsule->SetCollisionEnabled(WDashPreviousCollisionEnabled);
    }
}



void AChampion_Alistar::Skill_E()
{
    if (!IsLocallyControlled()) return;

    // ë§ˆìš°???€ê²ŸíŒ… ?†ìŒ ???ê¸° ì£¼ë????ì—­ ?¨ê³¼
    Server_Skill_E();
}

bool AChampion_Alistar::Server_Skill_E_Validate()
{
    return true;
}

void AChampion_Alistar::Server_Skill_E_Implementation()
{
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    if (ChampionResource.EMontage.IsValidIndex(AM_SKIll_E_IDX) &&
        ChampionResource.EMontage[AM_SKIll_E_IDX])
    {
        Multicast_PlayMontage(ChampionResource.EMontage[AM_SKIll_E_IDX], 1.0f);
    }
    UE_LOG(LogTemp, Warning, TEXT("[Alistar E] DoT started. Ticks=%d"), E_MaxTicks);

    // ??ì¹´ìš´??ì´ˆê¸°??
    E_CurrentTick = 0;

    // ??ë°˜ë³µ ?€?´ë¨¸ ??1ì´ˆë§ˆ??ApplyEDamageTick ?¸ì¶œ
    GetWorld()->GetTimerManager().SetTimer(
        E_TickTimerHandle,
        this,
        &AChampion_Alistar::ApplyEDamageTick,
        E_TickInterval,
        true   // ??true = ë°˜ë³µ!
    );

    // ì¦‰ì‹œ ??ë²?ë°œë™ (? íƒ?¬í•­ ??ì²??°ë?ì§€ê°€ 1ì´????¤ì–´ê°€ì§€ ë§ê³  ì¦‰ì‹œ)
    ApplyEDamageTick();
}

void AChampion_Alistar::ApplyEDamageTick()
{
    E_CurrentTick++;

UE_LOG(LogTemp, Log, TEXT("[Alistar E] Tick %d/%d"), E_CurrentTick, E_MaxTicks);

    // ì£¼ë? ??ê²€ì¶?
    FVector Center = GetActorLocation();
    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(E_Radius);

    bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        Center,
        Center,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (bHit)
    {
        // ?°ë?ì§€ ê³„ì‚°
        float TotalDamage = SkillComponent->GetE_Data().BaseDamage[0] +
            StatComponent->GetStat().AbilityPower * 7.0f;

        float SkillDamage = TotalDamage / E_MaxTicks;

        // ê°??ì—ê²??ìš©
        for (auto& Hit : Hits)
        {
            ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
            if (Target && IsEnemyActor(Target))
            {
                UGameplayStatics::ApplyDamage(
                    Target,
                    SkillDamage,
                    GetController(),
                    this,
                    ULOL_DamageMagic::StaticClass()
                );

                UE_LOG(LogTemp, Log, TEXT("[Alistar E] Damaged %s for %.0f"), *Target->GetName(), SkillDamage);
            }
        }
    }

    // ??ìµœë? ???„ë‹¬?˜ë©´ ?€?´ë¨¸ ?•ë¦¬
    if (E_CurrentTick >= E_MaxTicks)
    {
        GetWorldTimerManager().ClearTimer(E_TickTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] DoT finished"));

        // ??ì¶”ê? ???¤ìŒ ?‰í? ?¤í„´ ?Œë˜ê·?ON
        bNextAttackStun = true;
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] Next attack stun ready"));

        // 5ì´ˆê°„ ëª??°ë©´ ë§Œë£Œ
        GetWorld()->GetTimerManager().SetTimer(
            E_StunBuffTimerHandle,
            this,
            &AChampion_Alistar::EndStunBuff,
            E_StunBuffDuration,
            false
        );
    }
}

void AChampion_Alistar::EndStunBuff()
{
    if (bNextAttackStun)
    {
        bNextAttackStun = false;
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] Stun buff expired"));
    }
}

void AChampion_Alistar::OnBasicAttackHit(ACharacter* Target)
{
    if (!HasAuthority()) return;
    if (!IsValid(Target) || !IsEnemyActor(Target)) return;

    // ê°•í™” ?‰í? ?Œë˜ê·¸ê? ì¼œì ¸ ?ˆìœ¼ë©??¤í„´
    if (bNextAttackStun)
    {
        if (ABaseChampion* Enemy = Cast<ABaseChampion>(Target))
        {
            Enemy->ApplyStun(E_StunDuration);
            UE_LOG(LogTemp, Warning, TEXT("[Alistar E] Empowered attack stunned %s for %.1fs"), *Enemy->GetName(), E_StunDuration);
        }

        // ??ë²??°ê³  ?Œë˜ê·??„ê¸°
        bNextAttackStun = false;
        GetWorldTimerManager().ClearTimer(E_StunBuffTimerHandle);
    }
}

void AChampion_Alistar::Skill_R()
{
    if (!IsLocallyControlled()) return;

    // ë¡œì»¬ ?´ë¼?´ì–¸???œì ?ì„œ ?¤í„´ ?íƒœ?¬ë„ R?¤í‚¬?´ë©´ ?µê³¼
    if (bIsStunned && !CanCastWhileStunned('r')) return;
    Server_Skill_R();
}

bool AChampion_Alistar::Server_Skill_R_Validate()
{
    return true;
}

void AChampion_Alistar::Server_Skill_R_Implementation()
{
    if (bIsUltActive) return;  // ?´ë? ì¼œì ¸ ?ˆìœ¼ë©?ë¬´ì‹œ

    StartUlt();
    Multicast_PlayRMontage();
}

void AChampion_Alistar::StartUlt()
{
    bIsUltActive = true;

    Multicast_ClearCC();   // ??ClearCCExceptKnockup() ?€???´ê±¸ë¡?

    Multicast_SetRShieldVisible(true);

    UE_LOG(LogTemp, Warning, TEXT("[Alistar R] Ultimate started. Duration=%.1f"), UltDuration);

    GetWorld()->GetTimerManager().SetTimer(
        UltTimerHandle, this, &AChampion_Alistar::EndUlt, UltDuration, false);
}

void AChampion_Alistar::ClearCCExceptKnockup()
{
    // ?¤í„´ ?´ì œ
    if (bIsStunned)
    {
        ClearStun();
        UE_LOG(LogTemp, Log, TEXT("[Alistar R] Stun cleared"));
    }

    // ì§„í–‰ ì¤‘ì¸ ?¤í„´ ?€?´ë¨¸???•ë¦¬
    if (GetWorldTimerManager().IsTimerActive(StunHandle))
    {
        GetWorldTimerManager().ClearTimer(StunHandle);
    }

    if (bIsKnockedBack)
    {
        EndKnockback();
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
}

void AChampion_Alistar::EndUlt()
{
    bIsUltActive = false;
    Multicast_SetRShieldVisible(false);
    UE_LOG(LogTemp, Warning, TEXT("[Alistar R] Ultimate ended"));
}

void AChampion_Alistar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChampion_Alistar, bIsUltActive);
}

void AChampion_Alistar::Multicast_ClearCC_Implementation()
{
    // ?¤í„´ ?´ì œ
    if (bIsStunned) ClearStun();
    if (GetWorldTimerManager().IsTimerActive(StunHandle))
        GetWorldTimerManager().ClearTimer(StunHandle);

    // ?‰ë°±/?ì–´ë³??´ì œ (EndKnockback???Œë˜ê·??‰ë°± ?€?´ë¨¸ ?•ë¦¬)
    if (bIsKnockedBack)
    {
        EndKnockback();
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

float AChampion_Alistar::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // ê¶ê·¹ê¸??œì„± ì¤??¼í•´ 70% ê°ì†Œ (?¤ì œë¡œëŠ” 30%ë§?ë°›ìŒ)
    if (bIsUltActive)
    {
        DamageAmount *= (1.f - UltDamageReduction);
    }
    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool AChampion_Alistar::CanCastWhileStunned(uint8 skilltype) const
{
    return skilltype == 'r';
}
