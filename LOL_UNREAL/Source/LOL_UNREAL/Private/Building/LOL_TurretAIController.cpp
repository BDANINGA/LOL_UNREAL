#include "Building/LOL_TurretAIController.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_AttackComponent.h"

#include "Building/Building_Turret.h"

#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

ALOL_TurretAIController::ALOL_TurretAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALOL_TurretAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		// 0.2초마다 DecisionLoop 실행 (미니언 AI 구조 차용)
		GetWorld()->GetTimerManager().SetTimer(
			AI_DecisionTimer,
			this,
			&ALOL_TurretAIController::DecisionLoop,
			0.2f,
			true
		);
	}
}

void ALOL_TurretAIController::OnUnPossess()
{
	Super::OnUnPossess();
	GetWorld()->GetTimerManager().ClearTimer(AI_DecisionTimer);
}

void ALOL_TurretAIController::DecisionLoop()
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	ULOL_StatComponent* StatComp = ControlledPawn->FindComponentByClass<ULOL_StatComponent>();
	ULOL_StateComponent* StateComp = ControlledPawn->FindComponentByClass<ULOL_StateComponent>();
	ULOL_AttackComponent* AttackComp = ControlledPawn->FindComponentByClass<ULOL_AttackComponent>();

	if (!StatComp || !StateComp || !AttackComp) return;

	if (StateComp->HasStatusTag(LOLTags::State_Dead)) return;

	AActor* CurrentTarget = AttackComp->CombatTarget;
	if (IsValid(CurrentTarget))
	{
		ULOL_StateComponent* TargetState = CurrentTarget->FindComponentByClass<ULOL_StateComponent>();
		float Distance = ControlledPawn->GetDistanceTo(CurrentTarget);

		if ((TargetState && TargetState->HasStatusTag(LOLTags::State_Dead)) ||
			(Distance > StatComp->GetStat().AttackRange))
		{
			AttackComp->CombatTarget = nullptr;
			CurrentTarget = nullptr;

			if (ABuilding_Turret* Turret = Cast<ABuilding_Turret>(ControlledPawn))
			{
				Turret->CurrentDebugTarget = nullptr;
			}
		}
	}

	if (!IsValid(CurrentTarget))
	{
		AActor* NewTarget = ScanForClosestEnemy(StatComp->GetStat().AttackRange);
		if (NewTarget)
		{
			AttackComp->CombatTarget = NewTarget;
			CurrentTarget = NewTarget;
		}
	}

	if (IsValid(CurrentTarget) && AttackComp->bCanAttack)
	{
		AttackComp->StartAttack();
	}
	float Range = StatComp->GetStat().AttackRange;
}

AActor* ALOL_TurretAIController::ScanForClosestEnemy(float SearchRadius)
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return nullptr;

	ULOL_StateComponent* MyState = ControlledPawn->FindComponentByClass<ULOL_StateComponent>();
	if (!MyState) return nullptr;

	FVector MyLoc = ControlledPawn->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ControlledPawn);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps, MyLoc, FQuat::Identity, ECC_Pawn, Sphere, Params
	);

	AActor* BestTarget = nullptr;
	float MinDistSquared = FLT_MAX;

	if (bHit)
	{
		for (const FOverlapResult& Hit : Overlaps)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor) || HitActor == ControlledPawn) continue;

			ULOL_StateComponent* OtherState = HitActor->FindComponentByClass<ULOL_StateComponent>();
			if (!OtherState) continue;

			if (MyState->IsEnemy(OtherState))
			{
				float DistSq = FVector::DistSquared(MyLoc, HitActor->GetActorLocation());
				if (DistSq < MinDistSquared)
				{
					MinDistSquared = DistSq;
					BestTarget = HitActor;
				}
			}
		}
	}
	return BestTarget;
}