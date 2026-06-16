// 공격 관련 컴포넌트
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/StaticMeshComponent.h"

#include "BaseChampion.h"
#include "Minion/BaseMinion.h"
#include "Champion/Projectile/BaseProjectile.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ULOL_AttackComponent::ULOL_AttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
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
void ULOL_AttackComponent::UpdateAttackLogic()
{
    if (!OwnerPawn || !CombatTarget) return;

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

    float Distance = OwnerPawn->GetDistanceTo(CombatTarget);
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

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    if (!StateComp || !StatComp) return;

    bCanAttack = false;
    bHitHappened = false;

    StateComp->AddStatusTag(LOLTags::State_Attacking);
    StateComp->RemoveStatusTag(LOLTags::State_Moving);

    HitTarget = CombatTarget;

    if (OwnerPawn->HasAuthority())
    {
        FVector Direction = HitTarget->GetActorLocation() - OwnerPawn->GetActorLocation();
        Direction.Z = 0.f;

        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();

            if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
            {
                Champion->Multicast_SetTargetAndPlayMontage(
                    Champion->ChampionResource.AttackMontage[Champion->GetAM_Atk_Idx()],
                    StatComp->GetStat().AttackSpeed, NewRotation);
            }
            else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
            {
                // TODO: 미니언 멀티캐스트 몽타주 재생 로직 추가 필요
                Minion->SetActorRotation(NewRotation);
                if (Minion->MinionResource.AttackMontage.Num() > 0 && Minion->MinionResource.AttackMontage[0])
                {
                    if (UAnimInstance* AnimInst = Minion->GetMesh()->GetAnimInstance())
                    {
                        AnimInst->Montage_Play(Minion->MinionResource.AttackMontage[0], StatComp->GetStat().AttackSpeed);
                    }
                }
            }
        }
    }
    float AttackDelay = 1.0f / StatComp->GetStat().AttackSpeed;
    GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ULOL_AttackComponent::ResetAttack, AttackDelay, false);
}

void ULOL_AttackComponent::EndAttack()
{
    ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn);
    Champion->StateComponent->AddStatusTag(LOLTags::State_Moving);
    Champion->StateComponent->RemoveStatusTag(LOLTags::State_Attacking);
}

void ULOL_AttackComponent::ResetAttack()
{
    bCanAttack = true;
}

void ULOL_AttackComponent::ExecuteAttackHit()
{
    if (!OwnerPawn || !HitTarget) return;
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
                Arrow->SetMesh(Champion->ChampionResource.ProjectileMesh[0]);
            }

            FVector SpawnLocation = Champion->GetActorLocation() + (Champion->GetActorForwardVector() * 50.f);
            Arrow->Activate(SpawnLocation, HitTarget);
        }
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
void ULOL_AttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_AttackComponent, CombatTarget);
    DOREPLIFETIME(ULOL_AttackComponent, HitTarget);
}