#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LOL_GameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnOneSecondEvent);
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnChampionKillEvent,
	class ABaseChampion*,
	class ABaseChampion*);

UCLASS()
class LOL_UNREAL_API ALOL_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void TickOneSecond();

	// Uses PlayerState team IDs: 0 = undecided, 1 = blue, 2 = red.
	bool FinishMatch(uint8 InWinningTeamID);
	uint8 GetWinningTeamID() const { return WinningTeamID; }
	bool HasMatchFinished() const { return WinningTeamID != 0; }

	void AddTeamKill(bool bBlueTeam);
	void NotifyChampionKill(class ABaseChampion* Killer, class ABaseChampion* Victim);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyChampionKill(class ABaseChampion* Killer, class ABaseChampion* Victim);

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentMatchTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 BlueTeamKills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 RedTeamKills = 0;

	FOnOneSecondEvent OnOneSecondEvent;
	FOnChampionKillEvent OnChampionKill;

	FTimerHandle OneSecondTimerHandle;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match Result", meta = (AllowPrivateAccess = "true"))
	uint8 WinningTeamID = 0;
};
