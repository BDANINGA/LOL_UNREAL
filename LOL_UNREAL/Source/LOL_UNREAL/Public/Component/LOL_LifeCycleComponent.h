// 사망, 리스폰 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_LifeCycleComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_LifeCycleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_LifeCycleComponent();

	void Server_HandleDeath();
	
	UFUNCTION()
	void Respawn();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnRespawn();

private:
	UPROPERTY()
	class ABaseChampion* Owner;
};
