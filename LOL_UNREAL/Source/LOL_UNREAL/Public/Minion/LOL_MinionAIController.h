#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LOL_MinionAIController.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_MinionAIController : public AAIController
{
	GENERATED_BODY()
public:
	ALOL_MinionAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void DecisionLoop();

	AActor* ScanForClosestEnemy();
private:
	FTimerHandle AI_DecisionTimer;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float SearchRadius = 800.0f;

	FVector OriginalDestination;
};
