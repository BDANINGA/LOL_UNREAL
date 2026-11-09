// 이동 관련 컴포넌트
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

#include "BaseChampion.h"
#include "VisionManager/VisionManager.h"
#include "Kismet/GameplayStatics.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavigationPath.h"
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
        if(Champion->bIsStunned || Champion->bIsKnockedBack || Champion->IsGrabbed()) return;
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
                if (!EnemyState || EnemyState->HasStatusTag(LOLTags::State_Dead) || !Champion->IsEnemyActor(Enemy))
                {
                    Champion->EnemiesInRange.RemoveAt(i);
                    continue;
                }

                if (const auto* Manager = Cast<AVisionManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AVisionManager::StaticClass())))
                {
                    if (!Manager->CanTeamSeeActor(Champion, Enemy)) continue;
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
                Champion->AttackComponent->SetCombatTarget(BestTarget);
                return;
            }
        }

        const bool bShouldUpdateMovement =
            (StateComp && StateComp->HasStatusTag(LOLTags::State_Moving)) ||
            (Champion->IsLocallyControlled() && bUsingDirectMovement);

        if (bShouldUpdateMovement)
        {
            float Distance = FVector::Dist2D(OwnerPawn->GetActorLocation(), TargetLocation);
            if (Distance <= 100.f)
            {
                StopMovement();
            }
            else if (Champion->IsLocallyControlled() && bUsingDirectMovement)
            {
                while (LocalNavigationPath.IsValidIndex(CurrentNavigationPathIndex) &&
                    FVector::Dist2D(
                        OwnerPawn->GetActorLocation(),
                        LocalNavigationPath[CurrentNavigationPathIndex]) <= 60.0f)
                {
                    ++CurrentNavigationPathIndex;
                }

                const FVector MovementTarget =
                    LocalNavigationPath.IsValidIndex(CurrentNavigationPathIndex)
                    ? LocalNavigationPath[CurrentNavigationPathIndex]
                    : TargetLocation;
                FVector Direction = MovementTarget - OwnerPawn->GetActorLocation();
                Direction.Z = 0.0f;
                Champion->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
            }
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (!Minion->HasAuthority()) return;
        if (Minion->AttackComponent && Minion->AttackComponent->CombatTarget)
        {
            if (MovementTargetActor.IsValid())
            {
                TargetLocation = MovementTargetActor->GetActorLocation();
                if (bUsingDirectMovement)
                {
                    FVector Direction = TargetLocation - Minion->GetActorLocation();
                    Direction.Z = 0.f;
                    Minion->AddMovementInput(Direction.GetSafeNormal(), 1.f);
                }
            }
            return;
        }

        if (Minion->HasAuthority() && StateComp && StateComp->HasStatusTag(LOLTags::State_Moving) &&
            Minion->PathPoints.IsValidIndex(Minion->CurrentPathIndex) &&
            TargetLocation.Equals(Minion->PathPoints[Minion->CurrentPathIndex]))
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
    else if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn))
    {
        if (!JungleMonster->HasAuthority())
        {
            return;
        }
        if (!JungleMonster->StateComponent->HasStatusTag(LOLTags::State_Moving)) return;

        if (MovementTargetActor.IsValid())
        {
            TargetLocation = MovementTargetActor->GetActorLocation();
        }

        if (FVector::Dist2D(
            JungleMonster->GetActorLocation(),
            TargetLocation) <= 15.0f)
        {
            StopMovement();
        }
        else if (bUsingDirectMovement)
        {
            FVector Direction =
                TargetLocation - JungleMonster->GetActorLocation();
            Direction.Z = 0.0f;
            JungleMonster->AddMovementInput(
                Direction.GetSafeNormal(),
                1.0f
            );
            JungleMonster->SetActorRotation(Direction.Rotation());
        }
    }
}

void ULOL_MoveComponent::SetMoveTarget(FVector NewLocation, AActor* TargetActor)
{
    if (!OwnerPawn) return;
    if (const ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (Champion->IsGrabbed()) return;
    }
    TargetLocation = NewLocation;

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        const bool bIsAttackTarget =
            TargetActor &&
            TargetActor != Champion &&
            Champion->IsEnemyActor(TargetActor) &&
            TargetActor->FindComponentByClass<ULOL_StateComponent>();

        if (bIsAttackTarget) {
            Champion->StateComponent->RemoveStatusTag(LOLTags::State_Moving);
        }
        else {
            if (!Champion->StateComponent->HasStatusTag(LOLTags::State_Attacking))
            {
                Champion->StateComponent->AddStatusTag(LOLTags::State_Moving);
            }

            if (Champion->IsLocallyControlled())
            {
                if (Champion->HasAuthority())
                {
                    UNavigationSystemV1* NavigationSystem =
                        FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
                    const ANavigationData* NavigationData = NavigationSystem
                        ? NavigationSystem->GetDefaultNavDataInstance(
                            FNavigationSystem::DontCreate)
                        : nullptr;

                    if (NavigationData)
                    {
                        bUsingDirectMovement = false;
                        UAIBlueprintHelperLibrary::SimpleMoveToLocation(
                            Champion->GetController(),
                            TargetLocation
                        );
                    }
                    else
                    {
                        SetLocalNavigationPath({ TargetLocation });
                    }
                }
                else
                {
                    SetLocalNavigationPath({ TargetLocation });
                }
            }
            else if (Champion->HasAuthority())
            {
                TArray<FVector> PathPoints;
                if (UNavigationPath* NavigationPath =
                    UNavigationSystemV1::FindPathToLocationSynchronously(
                        this,
                        Champion->GetActorLocation(),
                        TargetLocation,
                        Champion))
                {
                    if (NavigationPath->IsValid() && !NavigationPath->IsPartial())
                    {
                        PathPoints = NavigationPath->PathPoints;
                    }
                }

                if (PathPoints.Num() < 2)
                {
                    PathPoints.Reset();
                    PathPoints.Add(TargetLocation);
                }

                Client_SetNavigationPath(PathPoints);
            }
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (!Minion->HasAuthority()) return;
        if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
            StateComp->AddStatusTag(LOLTags::State_Moving);

        AAIController* AICon = Cast<AAIController>(Minion->GetController());
        if (IsValid(TargetActor))
        {
            // Keep an active actor-follow request; rebuilding it every frame prevents progress.
            if (MovementTargetActor.Get() == TargetActor &&
                (bUsingDirectMovement || (AICon && AICon->GetMoveStatus() != EPathFollowingStatus::Idle)))
            {
                return;
            }

            MovementTargetActor = TargetActor;
            bUsingDirectMovement = true;
            if (AICon)
            {
                AICon->StopMovement();
                float TargetRadius = 0.f;
                if (const UCapsuleComponent* Capsule = TargetActor->FindComponentByClass<UCapsuleComponent>())
                {
                    TargetRadius = Capsule->GetScaledCapsuleRadius();
                }
                const float AttackRange = Minion->StatComponent->GetStat().AttackRange;
                float HorizontalReach = AttackRange + TargetRadius;
                if (Minion->AttackComponent->UsesCenterRangeAgainst(TargetActor))
                {
                    // Path following tests horizontal distance; reserve range for the height difference.
                    const float HeightDifference = TargetActor->GetActorLocation().Z - Minion->GetActorLocation().Z;
                    HorizontalReach = FMath::Sqrt(FMath::Max(0.f,
                        FMath::Square(AttackRange) - FMath::Square(HeightDifference)));
                }
                FAIMoveRequest Request(TargetActor);
                // Match the attack distance exactly; do not add the minion's radius a second time.
                Request.SetAcceptanceRadius(FMath::Max(0.f, HorizontalReach - 10.f));
                Request.SetReachTestIncludesAgentRadius(false);
                Request.SetReachTestIncludesGoalRadius(false);
                Request.SetUsePathfinding(true);
                Request.SetAllowPartialPath(true);
                bUsingDirectMovement = AICon->MoveTo(Request).Code == EPathFollowingRequestResult::Failed;
            }
            return;
        }

        MovementTargetActor.Reset();
        bUsingDirectMovement = false;
        if (AICon)
        {
            AICon->MoveToLocation(TargetLocation, 15.f);
        }
    }
    else if (ABaseJungleMonster* JungleMonster =
        Cast<ABaseJungleMonster>(OwnerPawn))
    {
        if (JungleMonster->IsStationaryMonster())
        {
            return;
        }

        if (ULOL_StateComponent* StateComp =
            OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
        {
            StateComp->AddStatusTag(LOLTags::State_Moving);
        }

        if (AAIController* AICon =
            Cast<AAIController>(JungleMonster->GetController()))
        {
            MovementTargetActor = TargetActor;

            EPathFollowingRequestResult::Type MoveResult =
                EPathFollowingRequestResult::Failed;
            if (TargetActor)
            {
                const ULOL_StatComponent* StatComponent =
                    JungleMonster->StatComponent;
                const float AcceptanceRadius = StatComponent
                    ? FMath::Max(
                        15.0f,
                        StatComponent->GetStat().AttackRange)
                    : 15.0f;
                MoveResult = AICon->MoveToActor(
                    TargetActor,
                    AcceptanceRadius,
                    true,
                    true,
                    true,
                    nullptr,
                    true
                );
            }
            else
            {
                MoveResult = AICon->MoveToLocation(
                    TargetLocation,
                    15.0f,
                    false,
                    true,
                    true,
                    true,
                    nullptr,
                    true
                );
            }

            bUsingDirectMovement =
                MoveResult != EPathFollowingRequestResult::RequestSuccessful;
        }
        else
        {
            MovementTargetActor = TargetActor;
            bUsingDirectMovement = true;
        }
    }
}

void ULOL_MoveComponent::StopMovement()
{
    if (!OwnerPawn) OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    bUsingDirectMovement = false;
    LocalNavigationPath.Reset();
    CurrentNavigationPathIndex = 0;
    MovementTargetActor.Reset();

    ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>();
    if (StateComp) StateComp->RemoveStatusTag(LOLTags::State_Moving);

    if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        TargetLocation = Champion->GetActorLocation();
        if (AController* PC = Champion->GetController()) PC->StopMovement();
        if (UCharacterMovementComponent* Movement = Champion->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
    else if (ABaseMinion* Minion = Cast<ABaseMinion>(OwnerPawn))
    {
        if (AAIController* AICon = Cast<AAIController>(Minion->GetController())) AICon->StopMovement();
        Minion->ConsumeMovementInputVector();
        if (UCharacterMovementComponent* Movement = Minion->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
    else if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn)) {
        TargetLocation = JungleMonster->GetActorLocation();
        if (AAIController* AICon =
            Cast<AAIController>(JungleMonster->GetController()))
        {
            AICon->StopMovement();
        }
        if (JungleMonster->GetCharacterMovement()) JungleMonster->GetCharacterMovement()->StopMovementImmediately();
    }
}

void ULOL_MoveComponent::SetLocalNavigationPath(
    const TArray<FVector>& NewPathPoints)
{
    LocalNavigationPath = NewPathPoints;
    CurrentNavigationPathIndex = 0;
    bUsingDirectMovement = LocalNavigationPath.Num() > 0;

    if (LocalNavigationPath.Num() > 0)
    {
        TargetLocation = LocalNavigationPath.Last();
    }
}

void ULOL_MoveComponent::Client_SetNavigationPath_Implementation(
    const TArray<FVector>& NewPathPoints)
{
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    if (NewPathPoints.IsEmpty())
    {
        StopMovement();
        bIsSearchAttack = false;
        return;
    }
    if (const ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
    {
        if (Champion->IsGrabbed()) return;
    }
    SetLocalNavigationPath(NewPathPoints);
}

void ULOL_MoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_MoveComponent, TargetLocation);
    DOREPLIFETIME(ULOL_MoveComponent, bIsSearchAttack);
}
