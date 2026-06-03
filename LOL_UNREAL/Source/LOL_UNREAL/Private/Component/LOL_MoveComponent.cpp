// 이동 관련 컴포넌트
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/SphereComponent.h"

#include "BaseChampion.h"
#include "Minion/BaseMinion.h"
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
    OwnerPawn = Cast<APawn>(GetOwner());
}

void ULOL_MoveComponent::UpdateMovement(float DeltaTime)
{
    if (!OwnerPawn) return;
    if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
    {
        if (StateComp->HasStatusTag(LOLTags::State_Dead)) return;
    }

    ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn);
    if (Champion)
    {
        if(Champion->bIsStunned || Champion->bIsKnockedBack) return;
        if (Champion->AttackComponent->CombatTarget) return;

        if (bIsSearchAttack && Champion->EnemiesInRange.Num() > 0)
        {
            AActor* BestTarget = nullptr;
            float MinDistSquared = FLT_MAX; // 루트 계산을 빼기 위해 제곱 거리 사용 (최적화)
            FVector MyLoc = Champion->GetActorLocation();

            for (int32 i = Champion->EnemiesInRange.Num() - 1; i >= 0; --i)
            {
                AActor* Enemy = Champion->EnemiesInRange[i];

                ULOL_StateComponent* EnemyState = Enemy->FindComponentByClass<ULOL_StateComponent>();
                if (!EnemyState || EnemyState->HasStatusTag(LOLTags::State_Dead))
                {
                    Champion->EnemiesInRange.RemoveAt(i);
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
                Champion->AttackComponent->CombatTarget = BestTarget;
                return;
            }
        }

        if (Champion->StateComponent->HasStatusTag(LOLTags::State_Moving))
        {
            FVector CurrentLocation = Champion->GetActorLocation();
            FVector Direction = TargetLocation - CurrentLocation;
            Direction.Z = 0.f;
            float Distance = Direction.Size();

            if (Distance <= 10.f) {
                StopMovement();
            }
            else {
                Champion->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
            }
        }
    }
    // 소유주가 미니언일 경우
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (!Minion->StateComponent->HasStatusTag(LOLTags::State_Moving)) return;

        FVector CurrentLocation = Minion->GetActorLocation();
        FVector Direction = TargetLocation - CurrentLocation;
        Direction.Z = 0.f;
        float Distance = Direction.Size();

        if (Distance <= 15.f) // 미니언은 약간의 오차 범위를 더 줍니다.
        {
            StopMovement();
        }
        else
        {
            float MoveSpeed = 300.f;
            FVector NewLocation = CurrentLocation + (Direction.GetSafeNormal() * MoveSpeed * DeltaTime);

            // 두 번째 인자 'true' (Sweep)는 미니언이 벽을 뚫고 가지 않도록 충돌 처리를 켜주는 옵션입니다.
            Minion->SetActorLocation(NewLocation, true);

            Minion->SetActorRotation(Direction.Rotation());
        }
    }
}

void ULOL_MoveComponent::SetMoveTarget(FVector NewLocation, AActor* TargetActor)
{
    if (!OwnerPawn) return;

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
        if (TargetChampion && TargetChampion != Champion) {
            Champion->StateComponent->RemoveStatusTag(LOLTags::State_Moving);
        }
        else {
            TargetLocation = NewLocation;
            if (!Champion->StateComponent->HasStatusTag(LOLTags::State_Attacking))
                Champion->StateComponent->AddStatusTag(LOLTags::State_Moving);
        }
    }
    else
    {
        TargetLocation = NewLocation;
        if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>()) {
            StateComp->AddStatusTag(LOLTags::State_Moving);
        }
    }
    
}

void ULOL_MoveComponent::StopMovement()
{
    if (!OwnerPawn) return;

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (StateComp) StateComp->RemoveStatusTag(LOLTags::State_Moving);

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn)) {
        if (Champion->GetCharacterMovement()) Champion->GetCharacterMovement()->StopMovementImmediately();
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn)) {
        TargetLocation = Minion->GetActorLocation();
    }
}

void ULOL_MoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_MoveComponent, TargetLocation);
    DOREPLIFETIME(ULOL_MoveComponent, bIsSearchAttack);
}
