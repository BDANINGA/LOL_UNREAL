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

    if (Owner->HasAuthority())
    {
        FVector Direction = Owner->CombatTarget->GetActorLocation() - Owner->GetActorLocation();
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

void ULOL_AttackComponent::ResetAttack()
{
    bCanAttack = true;
}

void ULOL_AttackComponent::ExecuteAttackHit()
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("ExecuteAttackHit"));
    if (!Owner || !Owner->CombatTarget || !Owner->StatComponent) return;
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("ExecuteAttackHit!!!!!"));
    bHitHappened = true;

    if (Owner->HasAuthority())
    {
        UGameplayStatics::ApplyDamage(
            Owner->CombatTarget,
            Owner->StatComponent->GetStat().AttackDamage,
            Owner->GetController(),
            Owner,
            nullptr
        );
        Owner->OnBasicAttackHit(Cast<ACharacter>(Owner->CombatTarget));
    }
}

void ULOL_AttackComponent::CancelAttack()
{
    if (!Owner) return;

    Owner->StopAnimMontage();
    if (!bHitHappened)
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);

        ResetAttack();
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
