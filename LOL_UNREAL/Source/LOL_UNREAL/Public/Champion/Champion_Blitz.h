#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Blitz.generated.h"

UCLASS()
class AChampion_Blitz : public ABaseChampion
{
	GENERATED_BODY()
	
public:
    AChampion_Blitz();

    virtual void Skill_Q() override;
    virtual void Skill_W() override;
    virtual void Skill_E() override;
    virtual void Skill_R() override;

protected:
    FTimerHandle UltTimerHandle;
};
