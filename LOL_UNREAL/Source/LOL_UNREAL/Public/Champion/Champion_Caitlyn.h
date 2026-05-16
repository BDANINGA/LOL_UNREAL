#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Caitlyn.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Caitlyn : public ABaseChampion
{
	GENERATED_BODY()
	
public:
	AChampion_Caitlyn();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_E() override;
	virtual void Skill_R() override;

protected:
	FTimerHandle UltTimerHandle;

};
