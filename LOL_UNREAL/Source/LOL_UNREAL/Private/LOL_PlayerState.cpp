#include "LOL_PLayerState.h"
#include "Net/UnrealNetwork.h"

void ALOL_PLayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALOL_PLayerState, Nickname);
    DOREPLIFETIME(ALOL_PLayerState, TeamID);
    DOREPLIFETIME(ALOL_PLayerState, SelectedChampion);
}
