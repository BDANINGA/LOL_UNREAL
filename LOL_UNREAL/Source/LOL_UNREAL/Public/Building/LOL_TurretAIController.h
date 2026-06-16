#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LOL_TurretAIController.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_TurretAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALOL_TurretAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void DecisionLoop();

	AActor* ScanForClosestEnemy(float SearchRadius);

private:
	FTimerHandle AI_DecisionTimer;
};