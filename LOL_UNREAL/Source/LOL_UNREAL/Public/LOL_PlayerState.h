#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/LOL_GameInstance.h"
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

	UPROPERTY(ReplicatedUsing = OnRep_PlayerData, BlueprintReadOnly, Category = "Player Data")
	FString Nickname;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerData, BlueprintReadOnly, Category = "Player Data")
	uint8 TeamID = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerData, BlueprintReadOnly, Category = "Player Data")
	EChampionID SelectedChampion = EChampionID::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Assists = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 MinionKills = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_PlayerData();
};
