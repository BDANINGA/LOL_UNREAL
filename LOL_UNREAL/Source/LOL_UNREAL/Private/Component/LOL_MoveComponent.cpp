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
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "Navigation/PathFollowingComponent.h"

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
    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (StateComp)
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

        if (StateComp && StateComp->HasStatusTag(LOLTags::State_Moving))
        {
            float Distance = FVector::Dist2D(OwnerPawn->GetActorLocation(), TargetLocation);
            if (Distance <= 50.f) {
                StopMovement();
            }
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (StateComp && StateComp->HasStatusTag(LOLTags::State_Moving))
        {
            float Distance = FVector::Dist2D(OwnerPawn->GetActorLocation(), TargetLocation);
            if (Distance <= 100.f)
            {
                StopMovement(); 
                if (Minion->CurrentPathIndex < Minion->PathPoints.Num() - 1)
                {
                    Minion->MoveToNextWaypoint();
                }
            }
        }
    }
}

void ULOL_MoveComponent::SetMoveTarget(FVector NewLocation, AActor* TargetActor)
{
    if (!OwnerPawn) return;
    TargetLocation = NewLocation;

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
        if (TargetChampion && TargetChampion != Champion) {
            Champion->StateComponent->RemoveStatusTag(LOLTags::State_Moving);
        }
        else {
            if (!Champion->StateComponent->HasStatusTag(LOLTags::State_Attacking))
            {
                Champion->StateComponent->AddStatusTag(LOLTags::State_Moving);
            }

            UAIBlueprintHelperLibrary::SimpleMoveToLocation(Champion->GetController(), TargetLocation);
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
            StateComp->AddStatusTag(LOLTags::State_Moving);

        if (AAIController* AICon = Cast<AAIController>(Minion->GetController()))
        {
            AICon->MoveToLocation(TargetLocation, 15.f);
        }
    }
}

void ULOL_MoveComponent::StopMovement()
{
    if (!OwnerPawn) return;

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (StateComp) StateComp->RemoveStatusTag(LOLTags::State_Moving);

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (AController* PC = Champion->GetController()) PC->StopMovement();
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (AAIController* AICon = Cast<AAIController>(Minion->GetController())) AICon->StopMovement();
    }
}

void ULOL_MoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_MoveComponent, TargetLocation);
    DOREPLIFETIME(ULOL_MoveComponent, bIsSearchAttack);
}
