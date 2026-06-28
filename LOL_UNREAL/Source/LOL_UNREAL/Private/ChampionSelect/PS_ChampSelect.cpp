#include "ChampionSelect/PS_ChampSelect.h"
#include "LOL_PlayerState.h"
#include "Lobby/LOL_GameInstance.h"
#include "GameFramework/PlayerController.h"
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
    SyncLocalGameInstance();
    OnChampSelectDataChanged.Broadcast();
}

void APS_ChampSelect::SyncLocalGameInstance()
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
        if (LockedChampion != EChampionID::None)
        {
            GameInstance->MySelectedChampion = LockedChampion;
        }
    }
}

void APS_ChampSelect::CopyProperties(APlayerState* NewPlayerState)
{
    Super::CopyProperties(NewPlayerState);

    // 새 맵에서 생성될 PS(본 게임용)로 캐스팅
    ALOL_PlayerState* NewPS = Cast<ALOL_PlayerState>(NewPlayerState);
    if (NewPS)
    {
        // 여기서 로비에서 결정한 값들을 넘겨줌
        NewPS->Nickname = this->Nickname;
        NewPS->TeamID = this->TeamID;
        NewPS->SelectedChampion = this->LockedChampion;
    }
}
