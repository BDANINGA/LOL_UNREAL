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
	bool ShouldReturnToWavePoint(class ABaseMinion* Minion) const;
	void ReturnToWavePoint(class ABaseMinion* Minion);
private:
	FTimerHandle AI_DecisionTimer;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float SearchRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChampionChaseLeashDistance = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float ChampionChaseResumeDistance = 250.0f;

	bool bReturningToWavePoint = false;
};
