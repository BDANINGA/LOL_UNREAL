#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Garen.generated.h"


UCLASS()
class LOL_UNREAL_API AChampion_Garen : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Garen();

	virtual void SetChampionData(FName RowName) override;

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

protected:
	FTimerHandle UltTimerHandle;
};
