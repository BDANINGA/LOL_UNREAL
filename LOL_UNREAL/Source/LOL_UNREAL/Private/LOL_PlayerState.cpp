#include "LOL_PlayerState.h"

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

void ALOL_PlayerState::AddMinionKill()
{
	if (HasAuthority())
	{
		++MinionKills;
	}
}

void ALOL_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALOL_PlayerState, Kills);
	DOREPLIFETIME(ALOL_PlayerState, Deaths);
	DOREPLIFETIME(ALOL_PlayerState, Assists);
	DOREPLIFETIME(ALOL_PlayerState, MinionKills);
}
