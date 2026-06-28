#include "ChampionSelect/PS_ChampSelect.h"
#include "Net/UnrealNetwork.h"

APS_ChampSelect::APS_ChampSelect()
{
    bReplicates = true;
}

void APS_ChampSelect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APS_ChampSelect, Nickname);
    DOREPLIFETIME(APS_ChampSelect, TeamID);
    DOREPLIFETIME(APS_ChampSelect, HoveredChampion);
    DOREPLIFETIME(APS_ChampSelect, LockedChampion);
    DOREPLIFETIME(APS_ChampSelect, bIsReady);
}

void APS_ChampSelect::OnRep_DataChanged()
{
    OnChampSelectDataChanged.Broadcast();
}

