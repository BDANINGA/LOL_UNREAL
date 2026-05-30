#include "Minion/LOL_MinionAIController.h"
#include "Minion/BaseMinion.h"
#include "Component/LOL_MoveComponent.h"

ALOL_MinionAIController::ALOL_MinionAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALOL_MinionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("미니언 AI 빙의 완료! (순수 C++)"));
	}

	ABaseMinion* Minion = Cast<ABaseMinion>(InPawn);
	if (Minion && Minion->MoveComponent)
	{
		FVector TestDestination = Minion->GetActorLocation() + FVector(3000.f, 0.f, 0.f);

		Minion->MoveComponent->SetMoveTarget(TestDestination, nullptr);
	}
}

void ALOL_MinionAIController::OnUnPossess()
{
	Super::OnUnPossess();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("미니언 AI 빙의 해제 (사망)"));
	}
}