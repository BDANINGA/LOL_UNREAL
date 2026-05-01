// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Alistar.generated.h"

/**
 * 
 */
UCLASS()
class LOL_UNREAL_API AChampion_Alistar : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Alistar();

	virtual void Skill_Q() override;
	virtual void Skill_W() override;

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")

	TObjectPtr<class ACharacter> CurrentWTarget;
	// 돌진 상태
	bool bIsW_Dashing = false;

	// 넉백 적용 함수
	void ApplyWKnockback(ACharacter* Target);
};
