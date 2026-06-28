#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LOL_PlayerState.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	void AddKill();
	void AddDeath();
	void AddAssist();
	void AddMinionKill();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Assists = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 MinionKills = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
