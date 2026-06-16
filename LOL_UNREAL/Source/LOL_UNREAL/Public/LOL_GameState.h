#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LOL_GameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnOneSecondEvent);

UCLASS()
class LOL_UNREAL_API ALOL_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void TickOneSecond();

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentMatchTime;

	FOnOneSecondEvent OnOneSecondEvent;

	FTimerHandle OneSecondTimerHandle;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
