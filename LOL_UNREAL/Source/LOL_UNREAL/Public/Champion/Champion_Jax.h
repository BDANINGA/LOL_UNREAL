#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Jax.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Jax : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Jax();

	virtual void SetChampionData(FName RowName) override;

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

protected:
	FTimerHandle UltTimerHandle;
};
