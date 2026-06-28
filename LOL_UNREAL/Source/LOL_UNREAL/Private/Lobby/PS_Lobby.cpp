#include "Lobby/PS_Lobby.h"
#include "Lobby/LOL_GameInstance.h"
#include "ChampionSelect/PS_ChampSelect.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

APS_Lobby::APS_Lobby()
{
    bReplicates = true;
    TeamID = 0;
}

void APS_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APS_Lobby, Nickname);
    DOREPLIFETIME(APS_Lobby, TeamID);
}
void APS_Lobby::OnRep_Nickname()
{
    SyncLocalGameInstance();
    OnPlayerDataReplicated.Broadcast();
}

void APS_Lobby::OnRep_TeamID()
{
    SyncLocalGameInstance();
    OnPlayerDataReplicated.Broadcast();
}

void APS_Lobby::SyncLocalGameInstance()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    if (ULOL_GameInstance* GameInstance =
        Cast<ULOL_GameInstance>(PlayerController->GetGameInstance()))
    {
        // Team assignment happens before the host submits a nickname.
        // Do not erase the nickname already saved on the GameStart map.
        if (!Nickname.IsEmpty())
        {
            GameInstance->MySavedNickname = Nickname;
        }
        GameInstance->MySavedTeamID = TeamID;
    }
}

void APS_Lobby::CopyProperties(APlayerState* NewPlayerState)
{
    Super::CopyProperties(NewPlayerState);

    // [디버깅] 여기서 값이 제대로인지 확인!
    UE_LOG(LogTemp, Warning, TEXT("CopyProperties: Nickname: %s, TeamID: %d"), *this->Nickname, this->TeamID);

    if (APS_ChampSelect* NewPS = Cast<APS_ChampSelect>(NewPlayerState))
    {
        NewPS->Nickname = this->Nickname;
        NewPS->TeamID = this->TeamID;
    }
}
