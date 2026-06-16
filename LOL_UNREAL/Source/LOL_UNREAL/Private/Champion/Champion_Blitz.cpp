#include "Champion/Champion_Blitz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

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
    if (!SkillComponent) return;

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

    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FVector LookDirection = TargetLocation - GetActorLocation();
    LookDirection.Z = 0.0f;
    if (LookDirection.IsNearlyZero()) return;

    const FRotator LookRotation = LookDirection.Rotation();
    const FVector SkillDirection = LookRotation.Vector();

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

    const float QRange = QData.Range[SkillLevelIdx];

    FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
    FVector End = Start + SkillDirection * QRange;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    const bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(70.0f),
        Params
    );

    if (!bHit) return;

    ABaseChampion* Target = Cast<ABaseChampion>(HitResult.GetActor());
    if (!Target || Target == this) return;

    GetWorldTimerManager().ClearTimer(PullTimerHandle);
    GetWorldTimerManager().ClearTimer(PullTimeoutTimerHandle);

    GrabbedTarget = Target;

    PullDestination = GetActorLocation() + SkillDirection * 95.0f;
    PullDestination.Z = GrabbedTarget->GetActorLocation().Z;

    if (GrabbedTarget->GetCharacterMovement())
    {
        GrabbedTarget->GetCharacterMovement()->StopMovementImmediately();
        GrabbedTarget->GetCharacterMovement()->DisableMovement();
    }

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
        0.5f,
        false
    );
}

void AChampion_Blitz::TickPullTarget()
{
    if (!GrabbedTarget)
    {
        GetWorldTimerManager().ClearTimer(PullTimerHandle);
        return;
    }

    FVector CurrentLocation = GrabbedTarget->GetActorLocation();

    float DistanceToDest = FVector::Dist2D(PullDestination, CurrentLocation);

    if (DistanceToDest <= 20.0f)
    {
        GrabbedTarget->SetActorLocation(PullDestination);

        if (GrabbedTarget->GetCharacterMovement())
        {
            GrabbedTarget->GetCharacterMovement()->SetDefaultMovementMode();
        }

        FRotator FinalRotation = (-GetActorForwardVector()).Rotation();
        FinalRotation.Pitch = 0.f;
        FinalRotation.Roll = 0.f;
        GrabbedTarget->SetActorRotation(FinalRotation);

        GetWorldTimerManager().ClearTimer(PullTimerHandle);
        GrabbedTarget = nullptr;
        return;
    }

    FVector NewLocation = FMath::VInterpConstantTo(
        CurrentLocation,
        PullDestination,
        0.01f,
        PullSpeed * 220.0f
    );

    GrabbedTarget->SetActorLocation(NewLocation);

    FRotator SmoothRotation = (-GetActorForwardVector()).Rotation();
    SmoothRotation.Pitch = 0.f;
    SmoothRotation.Roll = 0.f;
    GrabbedTarget->SetActorRotation(SmoothRotation);
}

void AChampion_Blitz::FinishPullTarget()
{
    GetWorldTimerManager().ClearTimer(PullTimerHandle);
    GetWorldTimerManager().ClearTimer(PullTimeoutTimerHandle);

    if (GrabbedTarget && GrabbedTarget->GetCharacterMovement())
    {
        GrabbedTarget->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    GrabbedTarget = nullptr;
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

    UE_LOG(LogTemp, Warning, TEXT("Blitz E Active"));
}

void AChampion_Blitz::ResetE()
{
    bIsEActive = false;
}

void AChampion_Blitz::OnAttackHitWithE(ABaseChampion* Target)
{
    if (!HasAuthority() || !Target) return;
    if (!bIsEActive) return;

    UE_LOG(LogTemp, Warning, TEXT("Blitz E KnockUp"));

    Target->StopAnimMontage();

    if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->CurrentRootMotion.Clear();
        MoveComp->SetMovementMode(MOVE_Falling);
    }

    Target->LaunchCharacter(FVector(0.f, 0.f, 500.f), true, true);

    ResetE();
}

void AChampion_Blitz::OnBasicAttackHit(ACharacter* Target)
{
    Super::OnBasicAttackHit(Target);

    UE_LOG(LogTemp, Warning, TEXT("Blitz OnBasicAttackHit Called. EActive=%d Target=%s"),
        bIsEActive,
        Target ? *Target->GetName() : TEXT("NULL"));

    if (!HasAuthority()) return;
    if (!bIsEActive) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target);
    if (!TargetChampion) return;

    OnAttackHitWithE(TargetChampion);
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
    if (!SkillComponent) return;

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

    TSet<ABaseChampion*> HitChampions;

    for (const FHitResult& Hit : Hits)
    {
        ABaseChampion* Target = Cast<ABaseChampion>(Hit.GetActor());
        if (!Target || Target == this || HitChampions.Contains(Target)) continue;
        if (Target->StateComponent->HasStatusTag(LOLTags::State_Dead)) continue;

        HitChampions.Add(Target);

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );

        Target->Multicast_ApplySilence(SilenceDuration);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Blitz R] Radius=%.1f Damage=%.1f Hit=%d"),
        Radius, SkillDamage, Hits.Num());
}