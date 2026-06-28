#include "LOL_GameState.h"
#include "Net/UnrealNetwork.h"

void ALOL_GameState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(OneSecondTimerHandle, this, &ALOL_GameState::TickOneSecond, 1.0f, true);
    }
}

void ALOL_GameState::TickOneSecond()
{
    OnOneSecondEvent.Broadcast();
    CurrentMatchTime += 1;
}

void ALOL_GameState::AddTeamKill(bool bBlueTeam)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bBlueTeam)
    {
        ++BlueTeamKills;
    }
    else
    {
        ++RedTeamKills;
    }
}

void ALOL_GameState::NotifyChampionKill(ABaseChampion* Killer, ABaseChampion* Victim)
{
    if (HasAuthority() && Killer && Victim)
    {
        Multicast_NotifyChampionKill(Killer, Victim);
    }
}

void ALOL_GameState::Multicast_NotifyChampionKill_Implementation(
    ABaseChampion* Killer,
    ABaseChampion* Victim)
{
    if (Killer && Victim)
    {
        OnChampionKill.Broadcast(Killer, Victim);
    }
}

void ALOL_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALOL_GameState, CurrentMatchTime);
    DOREPLIFETIME(ALOL_GameState, BlueTeamKills);
    DOREPLIFETIME(ALOL_GameState, RedTeamKills);
}
