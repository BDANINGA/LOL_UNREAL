// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Blitz.generated.h"

/**
 * 
 */
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
    virtual void Tick(float DeltaTime) override;

protected:
    // 스킬 변수와 함수는 나중에 추가
};
