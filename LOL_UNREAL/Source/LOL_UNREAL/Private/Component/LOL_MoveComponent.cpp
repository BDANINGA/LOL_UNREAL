// 이동 관련 컴포넌트
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SphereComponent.h"

#include "BaseChampion.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ULOL_MoveComponent::ULOL_MoveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULOL_MoveComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ABaseChampion>(GetOwner());
}

void ULOL_MoveComponent::UpdateMovement(float DeltaTime)
{
    if (!Owner || Owner->LifeCycleComponent->bIsDead || Owner->bIsStunned || Owner->bIsKnockedBack) return;

    if (Owner->CombatTarget) return; 

    if (bIsSearchAttack && Owner->EnemiesInRange.Num() > 0)
    {
        ABaseChampion* BestTarget = nullptr;
        float MinDistSquared = FLT_MAX; // 루트 계산을 빼기 위해 제곱 거리 사용 (최적화)
        FVector MyLoc = Owner->GetActorLocation();

        for (int32 i = Owner->EnemiesInRange.Num() - 1; i >= 0; --i)
        {
            ABaseChampion* Enemy = Owner->EnemiesInRange[i];

            if (!Enemy || Enemy->LifeCycleComponent->bIsDead)
            {
                Owner->EnemiesInRange.RemoveAt(i);
                continue;
            }

            float DistSquared = FVector::DistSquared(MyLoc, Enemy->GetActorLocation());
            if (DistSquared < MinDistSquared)
            {
                MinDistSquared = DistSquared;
                BestTarget = Enemy;
            }
        }
        if (BestTarget)
        {
            Owner->CombatTarget = BestTarget;
            return;
        }
    }

    if (bIsMoving)
    {
        FVector CurrentLocation = Owner->GetActorLocation();
        FVector Direction = TargetLocation - CurrentLocation;
        Direction.Z = 0.f;
        float Distance = Direction.Size();

        if (Distance <= 10.f) {
            StopMovement();
        }
        else {
            Owner->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
        }
    }
}

void ULOL_MoveComponent::SetMoveTarget(FVector NewLocation, AActor* TargetActor)
{
    ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
    if (TargetChampion && TargetChampion != Owner) {
        bIsMoving = false;
    }
    else {
        TargetLocation = NewLocation;
        bIsMoving = true;
    }
}

void ULOL_MoveComponent::StopMovement()
{
    bIsMoving = false;
    if (Owner && Owner->GetCharacterMovement())
    {
        Owner->GetCharacterMovement()->StopMovementImmediately();
    }
}

void ULOL_MoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_MoveComponent, TargetLocation);
    DOREPLIFETIME(ULOL_MoveComponent, bIsMoving);
    DOREPLIFETIME(ULOL_MoveComponent, bIsSearchAttack);
}
