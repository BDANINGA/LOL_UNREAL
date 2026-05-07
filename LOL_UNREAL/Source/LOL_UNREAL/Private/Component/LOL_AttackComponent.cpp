// 공격 관련 컴포넌트
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"

#include "BaseChampion.h"

#include "Kismet/GameplayStatics.h"

ULOL_AttackComponent::ULOL_AttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void ULOL_AttackComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ABaseChampion>(GetOwner());
}
void ULOL_AttackComponent::UpdateAttackLogic()
{
    if (!Owner || !Owner->CombatTarget || Owner->LifeCycleComponent->bIsDead || Owner == Owner->CombatTarget) return;

    float Distance = Owner->GetDistanceTo(Owner->CombatTarget);
    auto StatComp = Owner->StatComponent;

    // 사거리 안이면 공격
    if (StatComp && Distance <= StatComp->GetStat().AttackRange)
    {
        Owner->MoveComponent->bIsMoving = false;

        if (bCanAttack)
        {
            StartAttack();
        }
    }
    // 사거리 밖이면 추격
    else
    {
        Owner->MoveComponent->bIsMoving = true;
        Owner->MoveComponent->TargetLocation = Owner->CombatTarget->GetActorLocation();

        FVector Direction = Owner->MoveComponent->TargetLocation - Owner->GetActorLocation();
        Direction.Z = 0.f;
        Owner->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
    }
}

void ULOL_AttackComponent::StartAttack()
{
    if (!Owner || !bCanAttack || !Owner->CombatTarget || !Owner->StatComponent) return;

    bCanAttack = false;

    // 서버에서 데미지 계산 및 몽타주 재생
    if (Owner->HasAuthority())
    {
        UGameplayStatics::ApplyDamage(
            Owner->CombatTarget,
            Owner->StatComponent->GetStat().AttackDamage,
            Owner->GetController(),
            Owner,
            nullptr
        );

        FVector Direction = Owner->CombatTarget->GetActorLocation() - Owner->GetActorLocation();
        Direction.Z = 0.f;

        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
            Owner->Multicast_PlayAttackMontage(NewRotation);
        }
    }

    float AttackDelay = 1.0f / Owner->StatComponent->GetStat().AttackSpeed;
    GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &ULOL_AttackComponent::ResetAttack, AttackDelay, false);
}

void ULOL_AttackComponent::ResetAttack()
{
    bCanAttack = true;
}

void ULOL_AttackComponent::ReceivedCrowdControl()
{
    bCanAttack = false;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
    }
}
