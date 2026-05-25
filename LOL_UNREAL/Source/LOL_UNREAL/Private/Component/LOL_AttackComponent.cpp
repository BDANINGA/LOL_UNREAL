// 공격 관련 컴포넌트
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "BaseChampion.h"
#include "Champion/Projectile/BaseProjectile.h"

#include "Kismet/GameplayStatics.h"

ULOL_AttackComponent::ULOL_AttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void ULOL_AttackComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ABaseChampion>(GetOwner());

    for (int32 i = 0; i < PoolSize; ++i)
    {
        ABaseProjectile* SpawnedProj = GetWorld()->SpawnActor<ABaseProjectile>(ABaseProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
            if (SpawnedProj)
            {
                SpawnedProj->Deactivate();
                ProjectilePool.Add(SpawnedProj);
                SpawnedProj->SetShooter(Owner);
            }
            else
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("생성불가!"));
    }
}
void ULOL_AttackComponent::UpdateAttackLogic()
{
    if (!Owner || !Owner->CombatTarget || Owner->HasStatusTag(LOLTags::State_Dead) || Owner == Owner->CombatTarget) return;

    if (ABaseChampion* TargetChmapion = Cast<ABaseChampion>(Owner->CombatTarget)) {
        if (TargetChmapion->HasStatusTag(LOLTags::State_Dead))
        {
            Owner->CombatTarget = nullptr;
            return;
        }
    }

    float Distance = Owner->GetDistanceTo(Owner->CombatTarget);
    auto StatComp = Owner->StatComponent;

    // 사거리 안이면 공격
    if (StatComp && Distance <= StatComp->GetStat().AttackRange)
    {
        Owner->RemoveStatusTag(LOLTags::State_Moving);

        if (bCanAttack)
        {
            StartAttack();
        }
    }
    // 사거리 밖이면 추격
    else
    {
        Owner->AddStatusTag(LOLTags::State_Moving);
        Owner->MoveComponent->TargetLocation = Owner->CombatTarget->GetActorLocation();

        FVector Direction = Owner->MoveComponent->TargetLocation - Owner->GetActorLocation();
        Direction.Z = 0.f;
        Owner->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
    }
}

void ULOL_AttackComponent::StartAttack()
{
    if (!Owner || !Owner->CombatTarget || !Owner->StatComponent) return;

    bCanAttack = false;
    bHitHappened = false;

    Owner->AddStatusTag(LOLTags::State_Attacking);
    Owner->RemoveStatusTag(LOLTags::State_Moving);

    Owner->HitTarget = Owner->CombatTarget;

    if (Owner->HasAuthority())
    {
        FVector Direction = Owner->HitTarget->GetActorLocation() - Owner->GetActorLocation();
        Direction.Z = 0.f;

        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
            Owner->Multicast_SetTargetAndPlayMontage(
                Owner->ChampionResource.AttackMontage[Owner->GetAM_Atk_Idx()],
                Owner->StatComponent->GetStat().AttackSpeed,
                NewRotation);
        }
    }
    float AttackDelay = 1.0f / Owner->StatComponent->GetStat().AttackSpeed;
    GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ULOL_AttackComponent::ResetAttack, AttackDelay, false);
}

void ULOL_AttackComponent::EndAttack()
{
    Owner->AddStatusTag(LOLTags::State_Moving);
    Owner->RemoveStatusTag(LOLTags::State_Attacking);
}

void ULOL_AttackComponent::ResetAttack()
{
    bCanAttack = true;
}

void ULOL_AttackComponent::ExecuteAttackHit()
{
    if (!Owner || !Owner->HitTarget || !Owner->StatComponent) return;
    
    if (!Owner->HasStatusTag(LOLTags::Champion_Ranged)) 
    {
        bHitHappened = true;
    }

    if (Owner->HasAuthority())
    {
        UGameplayStatics::ApplyDamage(
            Owner->HitTarget,
            Owner->StatComponent->GetStat().AttackDamage,
            Owner->GetController(),
            Owner,
            nullptr
        );
        Owner->OnBasicAttackHit(Cast<ACharacter>(Owner->HitTarget));
    }
}

void ULOL_AttackComponent::ExecuteRangeAttackHit()
{
    if (!Owner || !Owner->HitTarget || !Owner->StatComponent) return;
    bHitHappened = true;
    ABaseProjectile* Arrow = GetProjectileFromPool();

    if (Arrow)
    {
        if (Owner->ChampionResource.ProjectileMesh.Num() > 0 && Owner->ChampionResource.ProjectileMesh[0])
        {
            Arrow->SetMesh(Owner->ChampionResource.ProjectileMesh[0]);
        }

        FVector SpawnLocation = Owner->GetActorLocation() + (Owner->GetActorForwardVector() * 50.f);
        Arrow->Activate(SpawnLocation, Owner->HitTarget);
    }
}

void ULOL_AttackComponent::CancelAttack()
{
    if (!Owner) return;

    
    if (!bHitHappened)
    {
        Owner->StopAnimMontage();
        ResetAttack();
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
        Owner->RemoveStatusTag(LOLTags::State_Attacking);
    }
}

void ULOL_AttackComponent::ReceivedCrowdControl()
{
    bCanAttack = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
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
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("풀에 남은 화살이 없습니다!"));
    return nullptr;
}