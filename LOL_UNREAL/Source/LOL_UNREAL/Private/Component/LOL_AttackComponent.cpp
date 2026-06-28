// 공격 관련 컴포넌트
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

#include "BaseChampion.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
#include "Building/Building_Turret.h"
#include "Champion/Projectile/BaseProjectile.h"
#include "Champion/Champion_Garen.h"
#include "Champion/Champion_Jax.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ULOL_AttackComponent::ULOL_AttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    bCanAttack = true;
}

void ULOL_AttackComponent::BeginPlay()
{
	Super::BeginPlay();
    OwnerPawn = Cast<APawn>(GetOwner());

    for (int32 i = 0; i < PoolSize; ++i)
    {
        ABaseProjectile* SpawnedProj = GetWorld()->SpawnActor<ABaseProjectile>(ABaseProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (SpawnedProj)
        {
            SpawnedProj->Deactivate();
            ProjectilePool.Add(SpawnedProj);
            SpawnedProj->SetShooter(OwnerPawn);
        }
    }
}

bool ULOL_AttackComponent::IsValidAttackTarget(AActor* Target) const
{
    if (!OwnerPawn || !Target || Target == OwnerPawn)
    {
        return false;
    }

    if (const ABaseChampion* OwnerChampion = Cast<ABaseChampion>(OwnerPawn))
    {
        return OwnerChampion->IsEnemyActor(Target);
    }

    ULOL_StateComponent* OwnerState = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    ULOL_StateComponent* TargetState = Target->FindComponentByClass<ULOL_StateComponent>();
    if (OwnerState && TargetState)
    {
        return OwnerState->IsEnemy(TargetState);
    }

    return false;
}

void ULOL_AttackComponent::SetCombatTarget(AActor* Target)
{
    CombatTarget = IsValidAttackTarget(Target) ? Target : nullptr;
}

void ULOL_AttackComponent::UpdateAttackLogic()
{
    if (!OwnerPawn || !CombatTarget) return;

    if (!IsValidAttackTarget(CombatTarget))
    {
        CombatTarget = nullptr;
        HitTarget = nullptr;
        return;
    }

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    ULOL_MoveComponent* MoveComp = OwnerPawn->FindComponentByClass<ULOL_MoveComponent>();

    if (!StateComp || !StatComp || !MoveComp) return;

    if (StateComp->HasStatusTag(LOLTags::State_Dead) || OwnerPawn == CombatTarget) return;

    if (ULOL_StateComponent* TargetStateComp = CombatTarget->FindComponentByClass<ULOL_StateComponent>())
    {
        if (TargetStateComp->HasStatusTag(LOLTags::State_Dead))
        {
            CombatTarget = nullptr;
            return;
        }
    }
    float TargetRadius = 0.f;
    if (UCapsuleComponent* TargetCap = CombatTarget->FindComponentByClass<UCapsuleComponent>())
    {
        TargetRadius = TargetCap->GetScaledCapsuleRadius();
    }

    float Distance = FMath::Max(0.f, OwnerPawn->GetDistanceTo(CombatTarget) - TargetRadius);
    // 사거리 안이면 공격
    if (Distance <= StatComp->GetStat().AttackRange)
    {
        MoveComp->StopMovement();

        if (bCanAttack)
        {
            StartAttack();
        }
    }
    // 사거리 밖이면 추격
    else
    {
        if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn))
        {
            if (JungleMonster->IsStationaryMonster())
            {
                MoveComp->StopMovement();
                StateComp->RemoveStatusTag(LOLTags::State_Moving);
                return;
            }
        }

        StateComp->AddStatusTag(LOLTags::State_Moving);
        MoveComp->TargetLocation = CombatTarget->GetActorLocation();

        FVector Direction = MoveComp->TargetLocation - OwnerPawn->GetActorLocation();
        Direction.Z = 0.f;
        OwnerPawn->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
    }
}

void ULOL_AttackComponent::StartAttack()
{
    if (!OwnerPawn || !CombatTarget) return;

    if (!IsValidAttackTarget(CombatTarget))
    {
        CombatTarget = nullptr;
        HitTarget = nullptr;
        return;
    }

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    if (!StateComp || !StatComp) return;

    bCanAttack = false;
    bHitHappened = false;
    GetWorld()->GetTimerManager().ClearTimer(AttackHitTimerHandle);

    StateComp->AddStatusTag(LOLTags::State_Attacking);
    StateComp->RemoveStatusTag(LOLTags::State_Moving);

    HitTarget = CombatTarget;
    const bool bUseTimedHitFallback =
        OwnerPawn->HasAuthority() &&
        !StateComp->HasStatusTag(LOLTags::Champion_Ranged);

    if (OwnerPawn->HasAuthority())
    {
        FVector Direction = HitTarget->GetActorLocation() - OwnerPawn->GetActorLocation();
        Direction.Z = 0.f;

        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();

            if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
            {
                const int32 AttackMontageIndex = Champion->GetAM_Atk_Idx();
                UAnimMontage* AttackMontage =
                    Champion->ChampionResource.AttackMontage.IsValidIndex(AttackMontageIndex)
                    ? Champion->ChampionResource.AttackMontage[AttackMontageIndex]
                    : nullptr;
                float AttackPlayRate = StatComp->GetStat().AttackSpeed;

                if (AChampion_Jax* Jax = Cast<AChampion_Jax>(Champion))
                {
                    if (Jax->IsWEmpowered())
                    {
                        if (UAnimMontage* WAttackMontage = Jax->GetWEmpoweredAttackMontage())
                        {
                            AttackMontage = WAttackMontage;
                            AttackPlayRate = Jax->GetWEmpoweredAttackPlayRate();
                        }
                    }
                }

                Champion->Multicast_SetTargetAndPlayMontage(
                    AttackMontage,
                    AttackPlayRate,
                    NewRotation
                );
            }
            else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
            {
                Minion->Multicast_SetTargetAndPlayMontage(Minion->MinionResource.AttackMontage[0],
                    StatComp->GetStat().AttackSpeed, NewRotation);   
            }
            else if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn))
            {
                JungleMonster->SetActorRotation(NewRotation);
                if (JungleMonster->JungleMonsterResource.AttackMontage.Num() > 0 && JungleMonster->JungleMonsterResource.AttackMontage[0])
                {
                    if (UAnimInstance* AnimInst = JungleMonster->GetMesh()->GetAnimInstance())
                    {
                        AnimInst->Montage_Play(JungleMonster->JungleMonsterResource.AttackMontage[0], StatComp->GetStat().AttackSpeed);
                    }
                }
            }
            else if (ABuilding_Turret* Turret = Cast<ABuilding_Turret>(OwnerPawn))
            {
                ExecuteRangeAttackHit();
            }
        }
    }
    float AttackDelay = 1.0f / StatComp->GetStat().AttackSpeed;
    if (bUseTimedHitFallback)
    {
        const float HitDelay = FMath::Clamp(AttackDelay * 0.35f, 0.1f, AttackDelay);
        GetWorld()->GetTimerManager().SetTimer(
            AttackHitTimerHandle,
            FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                if (!bHitHappened)
                {
                    ExecuteAttackHit();
                }
                EndAttack();
            }),
            HitDelay,
            false
        );
    }
    GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ULOL_AttackComponent::ResetAttack, AttackDelay, false);
}

void ULOL_AttackComponent::EndAttack()
{
    if (!OwnerPawn) return;

    if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->AddStatusTag(LOLTags::State_Moving);
        StateComp->RemoveStatusTag(LOLTags::State_Attacking);
    }
}

void ULOL_AttackComponent::ResetAttack()
{
    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (AChampion_Garen* Garen = Cast<AChampion_Garen>(Champion))
        {
            if (Garen->bIsSpinning)
            {
                bCanAttack = false;
                return;
            }
        }
    }

    bCanAttack = true;
}

void ULOL_AttackComponent::ExecuteAttackHit()
{
    if (!OwnerPawn || !HitTarget) return;
    if (!IsValidAttackTarget(HitTarget))
    {
        HitTarget = nullptr;
        return;
    }

    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (!StatComp || !StateComp) return;

    if (!StateComp->HasStatusTag(LOLTags::Champion_Ranged))
    {
        bHitHappened = true;
    }

    if (OwnerPawn->HasAuthority())
    {
        UGameplayStatics::ApplyDamage(
            HitTarget,
            StatComp->GetStat().AttackDamage,
            OwnerPawn->GetController(),
            OwnerPawn,
            nullptr
        );

        // 챔피언 특화 OnHit 함수 호출
        if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
        {
            Champion->OnBasicAttackHit(Cast<ACharacter>(HitTarget));
        }
    }
}

void ULOL_AttackComponent::ExecuteRangeAttackHit()
{
    if (!OwnerPawn || !HitTarget) return;
    if (!IsValidAttackTarget(HitTarget))
    {
        HitTarget = nullptr;
        return;
    }

    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (!StatComp || !StateComp) return;

    bHitHappened = true;
    ABaseProjectile* Arrow = GetProjectileFromPool();

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (Arrow)
        {
            if (Champion->ChampionResource.ProjectileMesh.Num() > 0 && Champion->ChampionResource.ProjectileMesh[0])
            {
                Arrow->SetShooter(Champion);
                Arrow->SetMesh(Champion->ChampionResource.ProjectileMesh[0]);
            }

            FVector SpawnLocation = Champion->GetActorLocation() + (Champion->GetActorForwardVector() * 50.f);
            Arrow->Activate(SpawnLocation, HitTarget);
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        UNiagaraSystem* TargetNiagara = nullptr;

        APlayerController* PC = Minion->GetWorld()->GetFirstPlayerController();
        if (PC && PC->GetPawn())
        {
            if (ABaseChampion* LocalPlayer = Cast<ABaseChampion>(PC->GetPawn()))
            {
                bool bIsEnemy = LocalPlayer->StateComponent->IsEnemy(Minion->StateComponent);
                TargetNiagara = bIsEnemy ? Minion->MinionResource.EnemyProjectileNiagara : Minion->MinionResource.AllyProjectileNiagara;
            }
        }

        if (TargetNiagara)
        {
            Arrow->SetShooter(Minion);
            Arrow->SetNiagara(TargetNiagara);
        }

        FVector SpawnLocation = Minion->GetActorLocation() + (Minion->GetActorForwardVector() * 50.f);
        Arrow->Activate(SpawnLocation, HitTarget);
    }
    else if (ABuilding_Turret* Turret = Cast<ABuilding_Turret>(OwnerPawn))
    {
        UNiagaraSystem* TargetNiagara = nullptr;
        APlayerController* PC = Turret->GetWorld()->GetFirstPlayerController();
        if (PC && PC->GetPawn())
        {
            if (ABaseChampion* LocalPlayer = Cast<ABaseChampion>(PC->GetPawn()))
            {
                ULOL_StateComponent* TStateComp = Turret->FindComponentByClass<ULOL_StateComponent>();
                bool bIsEnemy = LocalPlayer->StateComponent->IsEnemy(TStateComp);
                TargetNiagara = bIsEnemy ? Turret->GetEnemyProjectileNiagara() : Turret->GetAllyProjectileNiagara();
            }
        }

        if (TargetNiagara)
        {
            Arrow->SetShooter(Turret);
            Arrow->SetNiagara(TargetNiagara);
        }

        FVector SpawnLocation = Turret->GetActorLocation() + (Turret->GetActorForwardVector() * 150.f) + FVector(0.f, 0.f, 250.f);
        TArray<USceneComponent*> Components;
        Turret->GetComponents<USceneComponent>(Components);
        for (USceneComponent* Comp : Components)
        {
            if (Comp && Comp->GetName() == TEXT("FirePoint"))
            {
                SpawnLocation = Comp->GetComponentLocation();
                break;
            }
        }
        FVector TargetLocation = HitTarget->GetActorLocation();

        Arrow->Activate(SpawnLocation, HitTarget);
    }
}

void ULOL_AttackComponent::CancelAttack()
{
    if (!OwnerPawn) return;

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (!bHitHappened)
        {
            Champion->StopAnimMontage();
            ResetAttack();
            GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
            GetWorld()->GetTimerManager().ClearTimer(AttackHitTimerHandle);
            Champion->StateComponent->RemoveStatusTag(LOLTags::State_Attacking);
        }
    }
}

void ULOL_AttackComponent::ReceivedCrowdControl()
{
    bCanAttack = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(AttackHitTimerHandle);
    }
}
ABaseProjectile* ULOL_AttackComponent::GetProjectileFromPool()
{
    for (ABaseProjectile* Proj : ProjectilePool)
    {
        if (Proj && !Proj->bIsActive)
        {
            return Proj;
        }
    }
    return nullptr;
}
void ULOL_AttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_AttackComponent, CombatTarget);
    DOREPLIFETIME(ULOL_AttackComponent, HitTarget);
}
