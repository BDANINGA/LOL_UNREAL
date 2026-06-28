#include "ChampionSelect/PS_ChampSelect.h"
#include "LOL_PlayerState.h"
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

void APS_ChampSelect::CopyProperties(APlayerState* NewPlayerState)
{
    Super::CopyProperties(NewPlayerState);

    // 새 맵에서 생성될 PS(본 게임용)로 캐스팅
    ALOL_PLayerState* NewPS = Cast<ALOL_PLayerState>(NewPlayerState);
    if (NewPS)
    {
        // 여기서 로비에서 결정한 값들을 넘겨줌
        NewPS->Nickname = this->Nickname;
        NewPS->TeamID = this->TeamID;
        NewPS->SelectedChampion = this->LockedChampion;
    }
}
