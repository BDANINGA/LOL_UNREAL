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

	UFUNCTION()
	void Server_HandleDeath(AController* KillerInstigator, AActor* DamageCauser);
	
	UFUNCTION()
	void Respawn();

	void RecordDamageFrom(AController* DamageInstigator);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LifeCycle")
	bool bCanRespawn = true; // 챔피언은 true, 미니언/몬스터는 false로 세팅

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LifeCycle")
	float DespawnDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LifeCycle")
	float JungleRespawnDelay = 15.0f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnRespawn();

private:
	UPROPERTY()
	class APawn* OwnerPawn;

	TMap<TWeakObjectPtr<AController>, float> RecentDamageContributors;

	float AssistWindowSeconds = 10.0f;
};
