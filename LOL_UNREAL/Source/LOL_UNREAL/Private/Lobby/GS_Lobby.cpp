#include "Lobby/GS_Lobby.h"
#include "Net/UnrealNetwork.h"

void AGS_Lobby::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGS_Lobby, TeamListVersion);
}

void AGS_Lobby::NotifyTeamChanged()
{
    // 서버에서만 호출됨
    TeamListVersion++;

    // 서버 UI도 바로 갱신해야 하므로 여기서 바로 호출
    OnTeamListChanged.Broadcast();
}

void AGS_Lobby::OnRep_TeamList()
{
    // 이 함수는 클라이언트에서 변수가 업데이트될 때 자동으로 호출됨!
    OnTeamListChanged.Broadcast();
}