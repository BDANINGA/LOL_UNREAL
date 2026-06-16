#include "Champion/Champion_Ezreal.h"

#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/Champion_SkillComponent.h"
#include "BaseChampion.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Ezreal::AChampion_Ezreal()
{
    ChampionName = TEXT("Ezreal");
    SetChampionData(ChampionName);
}

void AChampion_Ezreal::Skill_Q()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_Q(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_W()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_W(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_E(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_R(Hit.ImpactPoint);
    }
}

bool AChampion_Ezreal::Server_Skill_Q_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_W_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_E_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_R_Validate(FVector TargetLocation) { return true; }

void AChampion_Ezreal::Server_Skill_Q_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    if (ChampionResource.QMontage.IsValidIndex(AM_SKIll_Q_IDX) &&
        ChampionResource.QMontage[AM_SKIll_Q_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.QMontage[AM_SKIll_Q_IDX],
            1.0f,
            LookRotation
        );
    }

    ApplyEzrealLineSkill(
        TargetLocation,
        QData,
        SkillLevelIdx,
        60.0f,
        nullptr,
        1.0f,
        false
    );
}

void AChampion_Ezreal::Server_Skill_W_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("W", 1)) return;

    FSkillData& WData = SkillComponent->GetW_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    if (ChampionResource.WMontage.IsValidIndex(AM_SKIll_W_IDX) &&
        ChampionResource.WMontage[AM_SKIll_W_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.WMontage[AM_SKIll_W_IDX],
            1.0f,
            LookRotation
        );
    }

    ApplyEzrealLineSkill(
        TargetLocation,
        WData,
        SkillLevelIdx,
        70.0f,
        ULOL_DamageMagic::StaticClass(),
        0.0f,
        false
    );
}

void AChampion_Ezreal::Server_Skill_E_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    FSkillData& EData = SkillComponent->GetE_Data();
    const int32 SkillLevelIdx = 0;

    const float Range = EData.Range.IsValidIndex(SkillLevelIdx)
        ? EData.Range[SkillLevelIdx]
        : 475.0f;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    Direction = Direction.GetSafeNormal();

    FVector BlinkLocation = GetActorLocation() + Direction * Range;
    BlinkLocation.Z = GetActorLocation().Z;

    if (FVector::Dist2D(GetActorLocation(), TargetLocation) < Range)
    {
        BlinkLocation = TargetLocation;
        BlinkLocation.Z = GetActorLocation().Z;
    }

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    if (ChampionResource.EMontage.IsValidIndex(AM_SKIll_E_IDX) &&
        ChampionResource.EMontage[AM_SKIll_E_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.EMontage[AM_SKIll_E_IDX],
            1.0f,
            LookRotation
        );
    }

    SetActorLocation(BlinkLocation, true);

    const float DamageRadius = 650.0f;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(DamageRadius),
        Params
    );

    ABaseChampion* NearestTarget = nullptr;
    float NearestDistSq = TNumericLimits<float>::Max();

    for (const FHitResult& Hit : Hits)
    {
        ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
        if (!TargetChampion || TargetChampion == this) continue;

        const float DistSq = FVector::DistSquared(GetActorLocation(), TargetChampion->GetActorLocation());
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestTarget = TargetChampion;
        }
    }

    if (NearestTarget)
    {
        const float BaseDamage = EData.BaseDamage.IsValidIndex(SkillLevelIdx)
            ? EData.BaseDamage[SkillLevelIdx]
            : 0.0f;

        const float ADRatio = EData.SecondaryValue.IsValidIndex(SkillLevelIdx)
            ? EData.SecondaryValue[SkillLevelIdx]
            : 0.75f;

        const float SkillDamage =
            BaseDamage + StatComponent->GetStat().AttackDamage * ADRatio;

        UGameplayStatics::ApplyDamage(
            NearestTarget,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}

void AChampion_Ezreal::Server_Skill_R_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("R", 1)) return;

    FSkillData& RData = SkillComponent->GetR_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    if (ChampionResource.RMontage.IsValidIndex(AM_SKIll_R_IDX) &&
        ChampionResource.RMontage[AM_SKIll_R_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.RMontage[AM_SKIll_R_IDX],
            1.0f,
            LookRotation
        );
    }

    ApplyEzrealLineSkill(
        TargetLocation,
        RData,
        SkillLevelIdx,
        140.0f,
        ULOL_DamageMagic::StaticClass(),
        0.9f,
        true
    );
}

void AChampion_Ezreal::Multicast_PlayEzrealSkillMontage_Implementation(
    UAnimMontage* Montage,
    float PlayRate,
    FRotator NewRotation
)
{
    SetActorRotation(NewRotation);

    if (Montage)
    {
        PlayAnimMontage(Montage, PlayRate);
    }
}

void AChampion_Ezreal::ApplyEzrealLineSkill(
    FVector TargetLocation,
    FSkillData& SkillData,
    int32 SkillLevelIdx,
    float Radius,
    TSubclassOf<UDamageType> DamageType,
    float ADRatio,
    bool bHitMultiple
)
{
    if (!StatComponent || !GetWorld()) return;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero()) return;

    Direction = Direction.GetSafeNormal();

    const float Range = SkillData.Range.IsValidIndex(SkillLevelIdx)
        ? SkillData.Range[SkillLevelIdx]
        : 1200.0f;

    const float BaseDamage = SkillData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? SkillData.BaseDamage[SkillLevelIdx]
        : 0.0f;

    const float SkillDamage =
        BaseDamage + StatComponent->GetStat().AttackDamage * ADRatio;

    FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    FVector End = Start + Direction * Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (bHitMultiple)
    {
        TArray<FHitResult> Hits;

        const bool bHit = GetWorld()->SweepMultiByChannel(
            Hits,
            Start,
            End,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(Radius),
            Params
        );

        if (!bHit) return;

        TSet<ABaseChampion*> DamagedTargets;

        for (const FHitResult& Hit : Hits)
        {
            ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
            if (!TargetChampion || TargetChampion == this) continue;
            if (DamagedTargets.Contains(TargetChampion)) continue;

            DamagedTargets.Add(TargetChampion);

            UGameplayStatics::ApplyDamage(
                TargetChampion,
                SkillDamage,
                GetController(),
                this,
                DamageType
            );
        }

        return;
    }

    FHitResult Hit;

    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(Radius),
        Params
    );

    if (!bHit) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
    if (!TargetChampion || TargetChampion == this) return;

    UGameplayStatics::ApplyDamage(
        TargetChampion,
        SkillDamage,
        GetController(),
        this,
        DamageType
    );
}