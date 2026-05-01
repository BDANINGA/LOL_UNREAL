// 공격 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_AttackComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_AttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_AttackComponent();

	void UpdateAttackLogic();

	void StartAttack();

	void ResetAttack();

	void ReceivedCrowdControl();

	bool CanAttack() const { return bCanAttack; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class ABaseChampion* Owner;

	FTimerHandle AttackTimerHandle;
	bool bCanAttack = true;
};
