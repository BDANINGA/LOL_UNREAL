#include "Minion/LOL_MinionAIController.h"
#include "Minion/BaseMinion.h"
#include "BaseChampion.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StateComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
ALOL_MinionAIController::ALOL_MinionAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALOL_MinionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABaseMinion* Minion = Cast<ABaseMinion>(InPawn);
	if (Minion)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AI_DecisionTimer,
			this,
			&ALOL_MinionAIController::DecisionLoop,
			0.2f,
			true
		);
	}
}

void ALOL_MinionAIController::OnUnPossess()
{
	Super::OnUnPossess();
	GetWorld()->GetTimerManager().ClearTimer(AI_DecisionTimer);
}
void ALOL_MinionAIController::DecisionLoop()
{
	ABaseMinion* Minion = Cast<ABaseMinion>(GetPawn());
	if (!Minion) return;

	if (Minion->StateComponent && Minion->StateComponent->HasStatusTag(LOLTags::State_Dead))
	{
		GetWorld()->GetTimerManager().ClearTimer(AI_DecisionTimer);
		return;
	}

	if (bReturningToWavePoint)
	{
		if (ShouldReturnToWavePoint(Minion))
		{
			ReturnToWavePoint(Minion);
			return;
		}

		bReturningToWavePoint = false;
	}

	if (Minion->AttackComponent &&
		Cast<ABaseChampion>(Minion->AttackComponent->CombatTarget) &&
		ShouldReturnToWavePoint(Minion))
	{
		bReturningToWavePoint = true;
		ReturnToWavePoint(Minion);
		return;
	}

	AActor* ClosestEnemy = ScanForClosestEnemy();

	if (ClosestEnemy)
	{
		if (Minion->AttackComponent)
		{
			Minion->AttackComponent->SetCombatTarget(ClosestEnemy);
		}
	}
	else
	{
		if (Minion->AttackComponent && Minion->AttackComponent->CombatTarget != nullptr)
		{
			Minion->AttackComponent->SetCombatTarget(nullptr);
		}

		if (Minion->MoveComponent)
		{
			if (Minion->PathPoints.IsValidIndex(Minion->CurrentPathIndex))
			{
				Minion->MoveComponent->SetMoveTarget(Minion->PathPoints[Minion->CurrentPathIndex], nullptr);
			}
		}
	}
}

bool ALOL_MinionAIController::ShouldReturnToWavePoint(ABaseMinion* Minion) const
{
	if (!Minion || !Minion->PathPoints.IsValidIndex(Minion->CurrentPathIndex))
	{
		return false;
	}

	const float AllowedDistance = bReturningToWavePoint
		? ChampionChaseResumeDistance
		: ChampionChaseLeashDistance;
	return FVector::Dist2D(
		Minion->GetActorLocation(),
		Minion->PathPoints[Minion->CurrentPathIndex]) > AllowedDistance;
}

void ALOL_MinionAIController::ReturnToWavePoint(ABaseMinion* Minion)
{
	if (!Minion || !Minion->PathPoints.IsValidIndex(Minion->CurrentPathIndex))
	{
		return;
	}

	if (Minion->AttackComponent)
	{
		GetWorldTimerManager().ClearTimer(Minion->AttackComponent->AttackHitTimerHandle);
		GetWorldTimerManager().ClearTimer(Minion->AttackComponent->AttackTimerHandle);
		Minion->AttackComponent->SetCombatTarget(nullptr);
		Minion->AttackComponent->HitTarget = nullptr;
		Minion->AttackComponent->bCanAttack = true;
	}

	if (Minion->StateComponent)
	{
		Minion->StateComponent->RemoveStatusTag(LOLTags::State_Attacking);
		Minion->StateComponent->AddStatusTag(LOLTags::State_Moving);
	}

	if (Minion->MoveComponent)
	{
		Minion->MoveComponent->SetMoveTarget(
			Minion->PathPoints[Minion->CurrentPathIndex],
			nullptr);
	}
}

AActor* ALOL_MinionAIController::ScanForClosestEnemy()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		GetWorldTimerManager().ClearTimer(AI_DecisionTimer);
		return nullptr;
	}

	FVector MyLoc = ControlledPawn->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ControlledPawn);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		MyLoc,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		Params
	);

	AActor* BestTarget = nullptr;
	float MinDistSquared = FLT_MAX;
	int32 BestPriority = 999;

	if (bHit)
	{
		for (const FOverlapResult& Hit : Overlaps)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor) || HitActor == ControlledPawn) continue;

			ULOL_StateComponent* MyState = ControlledPawn->FindComponentByClass<ULOL_StateComponent>();
			ULOL_StateComponent* TargetState = HitActor->FindComponentByClass<ULOL_StateComponent>();

			if (TargetState && !TargetState->HasStatusTag(LOLTags::State_Dead) && MyState && MyState->IsEnemy(TargetState))
			{
				int32 CurrentPriority = 999;
				if (Cast<ABaseMinion>(HitActor)) CurrentPriority = 1;
				else if (Cast<ABaseChampion>(HitActor)) CurrentPriority = 2;
				else CurrentPriority = 3;

				float DistSq = FVector::DistSquared(MyLoc, HitActor->GetActorLocation());

				if (CurrentPriority < BestPriority)
				{
					BestPriority = CurrentPriority;
					MinDistSquared = DistSq;
					BestTarget = HitActor;
				}
				else if (CurrentPriority == BestPriority && DistSq < MinDistSquared)
				{
					MinDistSquared = DistSq;
					BestTarget = HitActor;
				}
			}
		}
	}

	return BestTarget;
}
