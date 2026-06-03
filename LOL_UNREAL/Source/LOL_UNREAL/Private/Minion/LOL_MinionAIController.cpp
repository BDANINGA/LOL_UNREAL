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
#include "DrawDebugHelpers.h"

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
		OriginalDestination = Minion->GetActorLocation() + (Minion->GetActorForwardVector() * 5000.f);
		if (Minion->MoveComponent)
		{
			Minion->MoveComponent->SetMoveTarget(OriginalDestination, nullptr);
		}

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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("미니언 AI 빙의 해제 (사망)"));
	}
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
			Minion->MoveComponent->SetMoveTarget(OriginalDestination, nullptr);
		}
	}
}
AActor* ALOL_MinionAIController::ScanForClosestEnemy()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		// 미니언이 죽었으면 탐색 타이머를 멈추고 함수를 빠져나갑니다.
		GetWorldTimerManager().ClearTimer(AI_DecisionTimer); // (타이머 핸들 이름은 사장님 변수명에 맞게 변경하세요)
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
	float MinDistSquared = FLT_MAX; // 가장 가까운 적을 찾기 위한 비교 변수

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
				float DistSquared = FVector::DistSquared(MyLoc, HitActor->GetActorLocation());
				if (DistSquared < MinDistSquared)
				{
					MinDistSquared = DistSquared;
					BestTarget = HitActor;
				}
			}
		}
	}

	return BestTarget;
}