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
	APawn* Minion = GetPawn();
	if (!Minion) return nullptr;

	FVector MyLoc = Minion->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Minion);

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
			if (!HitActor) continue;

			// 대상이 죽었는지 체크
			ULOL_StateComponent* TargetState = HitActor->FindComponentByClass<ULOL_StateComponent>();
			if (TargetState && TargetState->HasStatusTag(LOLTags::State_Dead)) continue;

			// 챔피언이거나 다른 미니언인 경우 타겟으로 인정
			if (HitActor->IsA(ABaseChampion::StaticClass()) || HitActor->IsA(ABaseMinion::StaticClass()))
			{
				// TODO: 여기에 나중에 "같은 팀이면 무시" 로직이 들어가야 합니다!

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