#include "Lobby/PS_Lobby.h"
#include "ChampionSelect/PS_ChampSelect.h"
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
    OnPlayerDataReplicated.Broadcast();
}

void APS_Lobby::OnRep_TeamID()
{
    OnPlayerDataReplicated.Broadcast();
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