#include "Champion/Champion_Blitz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Champion/Projectile/BlitzQProjectile.h"
#include "Engine/StaticMesh.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> QProjectileMeshAsset(
        TEXT("/Game/Level/blitzcrank/blitz_tex/blitz_q.blitz_q")
    );
    if (QProjectileMeshAsset.Succeeded())
    {
        QProjectileMesh = QProjectileMeshAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Blitz Q projectile mesh failed to load."));
    }

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 330.0f;
	}
}

void AChampion_Blitz::Skill_Q()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult HitResult;
    if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

    Server_Skill_Q(HitResult.ImpactPoint);
}


bool AChampion_Blitz::Server_Skill_Q_Validate(FVector TargetLocation)
{
    return true;
}

void AChampion_Blitz::Server_Skill_Q_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !GetWorld() || IsValid(ActiveQProjectile) || IsValid(GrabbedTarget)) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;

    if (!QData.Range.IsValidIndex(SkillLevelIdx) ||
        !QData.Cooldown.IsValidIndex(SkillLevelIdx))
    {
        UE_LOG(LogTemp, Error, TEXT("Blitz Q skill data not loaded. RangeNum=%d CooldownNum=%d"),
            QData.Range.Num(),
            QData.Cooldown.Num());
        return;
    }

    FVector LookDirection = TargetLocation - GetActorLocation();
    LookDirection.Z = 0.0f;
    if (LookDirection.IsNearlyZero()) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    const FRotator LookRotation = LookDirection.Rotation();

    SetActorRotation(LookRotation);

    if (ChampionResource.QMontage.IsValidIndex(AM_SKIll_Q_IDX) &&
        ChampionResource.QMontage[AM_SKIll_Q_IDX])
    {
        Multicast_SetTargetAndPlayMontage(
            ChampionResource.QMontage[AM_SKIll_Q_IDX],
            1.0f,
            LookRotation
        );
    }

    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 80.f);
    const FTransform SpawnTransform(LookRotation, Start);
    ActiveQProjectile = GetWorld()->SpawnActorDeferred<ABlitzQProjectile>(
        ABlitzQProjectile::StaticClass(), SpawnTransform, this, this,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!ActiveQProjectile) return;

    ActiveQProjectile->Initialize(this, QProjectileSpeed, QData.Range[SkillLevelIdx],
        Q_Radius > 0.0f ? Q_Radius : 50.0f, QProjectileMesh,
        QProjectileVisualRadius, QProjectileVisualScale, QProjectileRotationOffset);
    ActiveQProjectile->FinishSpawning(SpawnTransform);
}

void AChampion_Blitz::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ActiveQProjectile))
    {
        ActiveQProjectile->Destroy();
    }
    ActiveQProjectile = nullptr;
    if (HasAuthority())
    {
        FinishPullTarget();
    }
    Super::EndPlay(EndPlayReason);
}

FVector AChampion_Blitz::GetSafePullDestination() const
{
    const float TargetRadius = GrabbedTarget->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float SafeDistance = FMath::Max(Q_PullDistance,
        GetCapsuleComponent()->GetScaledCapsuleRadius() + TargetRadius + 10.0f);
    FVector Destination = GetActorLocation() + PullDirection * SafeDistance;
    Destination.Z = GrabbedTarget->GetActorLocation().Z;
    return Destination;
}

void AChampion_Blitz::TickPullTarget()
{
    if (!IsValid(GrabbedTarget) || !IsValidBlitzSkillTarget(GrabbedTarget))
    {
        FinishPullTarget();
        return;
    }

    // Follow Blitz's current position so walking forward cannot overlap the landing point.
    PullDestination = GetSafePullDestination();
    const FVector NewLocation = FMath::VInterpConstantTo(
        GrabbedTarget->GetActorLocation(), PullDestination, 0.01f, PullSpeed * 220.0f);
    GrabbedTarget->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
    GrabbedTarget->SetActorRotation((-PullDirection).Rotation());
    GrabbedTarget->ForceNetUpdate();

    if (NewLocation.Equals(PullDestination, 0.1f))
    {
        FinishPullTarget();
    }
}

void AChampion_Blitz::FinishPullTarget()
{
    GetWorldTimerManager().ClearTimer(PullTimerHandle);
    GetWorldTimerManager().ClearTimer(PullTimeoutTimerHandle);
    if (!IsValid(GrabbedTarget))
    {
        GrabbedTarget = nullptr;
        return;
    }

    const ULOL_StateComponent* TargetState = GrabbedTarget->FindComponentByClass<ULOL_StateComponent>();
    const bool bAlive = !TargetState || !TargetState->HasStatusTag(LOLTags::State_Dead);
    if (bAlive)
    {
        const FVector SafeDestination = GetSafePullDestination();
        const float SafeDistance = FVector::Dist2D(SafeDestination, GetActorLocation());
        // Also protect the timeout/interrupt path if Blitz moved into the target this frame.
        if (FVector::Dist2D(GrabbedTarget->GetActorLocation(), GetActorLocation()) < SafeDistance)
        {
            FVector Away = (GrabbedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
            if (Away.IsNearlyZero()) Away = PullDirection;
            FVector ReleaseLocation = GetActorLocation() + Away * SafeDistance;
            ReleaseLocation.Z = GrabbedTarget->GetActorLocation().Z;
            GrabbedTarget->SetActorLocation(ReleaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
        }
        RestoreGrabbedTargetCollision();
    }

    if (ABaseChampion* Champion = Cast<ABaseChampion>(GrabbedTarget))
    {
        Champion->SetIsGrabbed(false);
    }
    if (ULOL_MoveComponent* Move = GrabbedTarget->FindComponentByClass<ULOL_MoveComponent>())
    {
        Move->StopMovement();
        Move->bIsSearchAttack = false;
    }
    GrabbedTarget->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement = GrabbedTarget->GetCharacterMovement())
    {
        Movement->ClearAccumulatedForces();
        Movement->StopMovementImmediately();
        if (bAlive && Movement->MovementMode == MOVE_None)
        {
            Movement->SetMovementMode(GrabbedTargetPreviousMovementMode, GrabbedTargetPreviousCustomMode);
        }
    }
    GrabbedTarget->ForceNetUpdate();
    GrabbedTarget = nullptr;
}

void AChampion_Blitz::BeginPullTarget(ACharacter* Target)
{
    if (!HasAuthority() || !SkillComponent || IsValid(GrabbedTarget) || !IsValidBlitzSkillTarget(Target)) return;
    if (const ABaseChampion* Champion = Cast<ABaseChampion>(Target))
    {
        if (Champion->IsGrabbed()) return;
    }

    const FSkillData& QData = SkillComponent->GetQ_Data();
    const float BaseDamage = QData.BaseDamage.IsValidIndex(0)
        ? QData.BaseDamage[0]
        : 0.0f;
    const float AbilityPower = StatComponent
        ? StatComponent->GetStat().AbilityPower
        : 0.0f;
    const float SkillDamage = BaseDamage + AbilityPower * Q_APRatio;

    if (SkillDamage > 0.0f)
    {
        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
    // A lethal hit must not disable movement/collision on an already dead target.
    if (!IsValidBlitzSkillTarget(Target)) return;
    GrabbedTarget = Target;

    PullDirection = GetActorForwardVector().GetSafeNormal2D();
    PullDestination = GetSafePullDestination();
    if (ABaseChampion* Champion = Cast<ABaseChampion>(Target))
    {
        Champion->SetIsGrabbed(true);
    }
    if (ULOL_MoveComponent* Move = Target->FindComponentByClass<ULOL_MoveComponent>())
    {
        Move->StopMovement();
        Move->bIsSearchAttack = false;
    }
    if (AController* TargetController = Target->GetController()) TargetController->StopMovement();
    Target->ConsumeMovementInputVector();
    Target->StopAnimMontage();

    if (UCapsuleComponent* Capsule = GrabbedTarget->GetCapsuleComponent())
    {
        GrabbedTargetPreviousCollisionEnabled = Capsule->GetCollisionEnabled();
        GrabbedTargetPreviousPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (UCharacterMovementComponent* Movement = Target->GetCharacterMovement())
    {
        GrabbedTargetPreviousMovementMode = Movement->MovementMode;
        GrabbedTargetPreviousCustomMode = Movement->CustomMovementMode;
        Movement->ClearAccumulatedForces();
        Movement->CurrentRootMotion.Clear();
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
    Target->ForceNetUpdate();

    GetWorldTimerManager().SetTimer(
        PullTimerHandle,
        this,
        &AChampion_Blitz::TickPullTarget,
        0.01f,
        true
    );

    GetWorldTimerManager().SetTimer(
        PullTimeoutTimerHandle,
        this,
        &AChampion_Blitz::FinishPullTarget,
        0.8f,
        false
    );
}

void AChampion_Blitz::RestoreGrabbedTargetCollision()
{
    if (GrabbedTarget)
    {
        if (UCapsuleComponent* Capsule = GrabbedTarget->GetCapsuleComponent())
        {
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, GrabbedTargetPreviousPawnResponse);
            Capsule->SetCollisionEnabled(GrabbedTargetPreviousCollisionEnabled);
        }
    }
}

bool AChampion_Blitz::IsValidBlitzSkillTarget(AActor* TargetActor) const
{
    if (!IsValid(TargetActor) || TargetActor == this) return false;

    const bool bSupportedTarget =
        Cast<ABaseChampion>(TargetActor) ||
        Cast<ABaseMinion>(TargetActor) ||
        Cast<ABaseJungleMonster>(TargetActor);
    if (!bSupportedTarget) return false;

    ULOL_StateComponent* TargetState = TargetActor->FindComponentByClass<ULOL_StateComponent>();
    if (!TargetState || TargetState->HasStatusTag(LOLTags::State_Dead)) return false;

    return IsEnemyActor(TargetActor);
}

void AChampion_Blitz::Skill_W() 
{
	if (!IsLocallyControlled()) return;
	if (bIsStunned || bIsKnockedBack) return;

	Server_Skill_W();
}

bool AChampion_Blitz::Server_Skill_W_Validate() { return true; }

void AChampion_Blitz::Server_Skill_W_Implementation()
{
    
    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().ClearTimer(W_SlowTimerHandle);

    if (GetCharacterMovement())
    { 
        GetCharacterMovement()->MaxWalkSpeed = 330.0f + (330.0f * W_SpeedBuffAmount);
    }

    GetWorldTimerManager().SetTimer(W_BuffTimerHandle, this, &AChampion_Blitz::EndWBuff, W_Duration, false);
}

void AChampion_Blitz::EndWBuff()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f - (330.0f * W_SlowAmount);
    }

    GetWorldTimerManager().SetTimer(W_SlowTimerHandle, this, &AChampion_Blitz::EndWSlow, W_SlowDuration, false);
}

void AChampion_Blitz::EndWSlow()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f;
    }
}

// ==========================================
// E 
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
    if (!SkillComponent) return;
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    bIsEActive = true;
}

void AChampion_Blitz::ResetE()
{
    bIsEActive = false;
}

void AChampion_Blitz::OnAttackHitWithE(ACharacter* Target)
{
    if (!HasAuthority() || !Target) return;
    if (!bIsEActive) return;
    if (!IsValidBlitzSkillTarget(Target)) return;

    Target->StopAnimMontage();

    if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->CurrentRootMotion.Clear();
        MoveComp->SetMovementMode(MOVE_Falling);
    }

    const float Damage = StatComponent
        ? StatComponent->GetStat().AttackDamage * E_DamageMultiplier
        : 0.0f;

    if (Damage > 0.0f)
    {
        UGameplayStatics::ApplyDamage(
            Target,
            Damage,
            GetController(),
            this,
            ULOL_DamagePhysical::StaticClass()
        );
    }

    Target->LaunchCharacter(FVector(0.f, 0.f, 500.f), true, true);

    ResetE();
}
void AChampion_Blitz::OnBasicAttackHit(ACharacter* Target)
{
    Super::OnBasicAttackHit(Target);

    if (!HasAuthority()) return;
    if (!bIsEActive) return;
    if (!Target) return;

    OnAttackHitWithE(Target);
}
void AChampion_Blitz::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_R();
}

bool AChampion_Blitz::Server_Skill_R_Validate() { return true; }

void AChampion_Blitz::Server_Skill_R_Implementation()
{
    if (!SkillComponent || !GetWorld()) return;

    FSkillData& RData = SkillComponent->GetR_Data();

    int32 SkillLevelIdx = 0;

    if (!RData.ManaCost.IsValidIndex(SkillLevelIdx)) return;
    if (!RData.Cooldown.IsValidIndex(SkillLevelIdx)) return;

    if (!SkillComponent->TryCastSkill("R", 1)) return;

    float Radius = RData.Range.IsValidIndex(SkillLevelIdx) ? RData.Range[SkillLevelIdx] : 600.0f;
    if (Radius <= 0.f) Radius = 600.0f;
    float SilenceDuration = RData.Duration.IsValidIndex(SkillLevelIdx) ? RData.Duration[SkillLevelIdx] : 0.5f;
    float BaseDamage = RData.BaseDamage.IsValidIndex(SkillLevelIdx) ? RData.BaseDamage[SkillLevelIdx] : 0.0f;
    float SkillDamage = BaseDamage + StatComponent->GetStat().AbilityPower * R_APRatio;

    if (ChampionResource.RMontage.IsValidIndex(AM_SKIll_R_IDX))
    {
        Multicast_PlayMontage(ChampionResource.RMontage[AM_SKIll_R_IDX], 1.0f);
    }

    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (!bHit) return;

    TSet<AActor*> HitTargets;

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!IsValidBlitzSkillTarget(Target) || HitTargets.Contains(Target)) continue;

        HitTargets.Add(Target);

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );

        if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
        {
            TargetChampion->Multicast_ApplySilence(SilenceDuration);
        }
    }

}
