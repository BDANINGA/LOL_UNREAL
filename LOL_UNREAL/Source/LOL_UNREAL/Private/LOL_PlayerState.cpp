#include "LOL_PlayerState.h"

#include "GameFramework/PlayerController.h"
#include "Lobby/LOL_GameInstance.h"
#include "Net/UnrealNetwork.h"

void ALOL_PlayerState::AddKill()
{
	if (HasAuthority())
	{
		++Kills;
	}
}

void ALOL_PlayerState::AddDeath()
{
	if (HasAuthority())
	{
		++Deaths;
	}
}

void ALOL_PlayerState::AddAssist()
{
	if (HasAuthority())
	{
		++Assists;
	}
}

void ALOL_PlayerState::AddMinionKill(int32 Amount)
{
	if (HasAuthority() && Amount > 0)
	{
		MinionKills += Amount;
	}
}

void ALOL_PlayerState::OnRep_PlayerData()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (ULOL_GameInstance* GameInstance =
		Cast<ULOL_GameInstance>(PlayerController->GetGameInstance()))
	{
		GameInstance->MySavedNickname = Nickname;
		GameInstance->MySavedTeamID = TeamID;
		GameInstance->MySelectedChampion = SelectedChampion;
	}
}

void ALOL_PlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALOL_PlayerState, Nickname);
	DOREPLIFETIME(ALOL_PlayerState, TeamID);
	DOREPLIFETIME(ALOL_PlayerState, SelectedChampion);
	DOREPLIFETIME(ALOL_PlayerState, Kills);
	DOREPLIFETIME(ALOL_PlayerState, Deaths);
	DOREPLIFETIME(ALOL_PlayerState, Assists);
	DOREPLIFETIME(ALOL_PlayerState, MinionKills);
}
