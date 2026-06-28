#include "Lobby/PC_Lobby.h"
#include "Lobby/PS_Lobby.h"
#include "Lobby/GM_Lobby.h"
#include "Lobby/LOL_GameInstance.h"

#include "Blueprint/UserWidget.h"

void APC_Lobby::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 클라이언트인 경우, GameInstance에서 닉네임을 가져와 서버로 보냅니다.
    if (IsLocalController())
    {
        if (LobbyWidgetClass)
        {
            LobbyWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
            if (LobbyWidget)
            {
                LobbyWidget->AddToViewport();

                // 마우스 커서 보이게 하기
                bShowMouseCursor = true;
                SetInputMode(FInputModeUIOnly());
            }
        }
        if (ULOL_GameInstance* GI = Cast<ULOL_GameInstance>(GetGameInstance()))
        {
            Server_SetNickname(GI->MySavedNickname);
        }
    }
}

void APC_Lobby::Server_SetNickname_Implementation(const FString& NewNickname)
{
    if (APS_Lobby* PS = GetPlayerState<APS_Lobby>())
    {
        PS->Nickname = NewNickname; // 서버에서 값 변경 -> 모든 클라이언트로 리플리케이트됨
    }
}

void APC_Lobby::Server_RequestChangeTeam_Implementation(uint8 TargetTeamID)
{
    // 서버의 GameMode에게 팀 변경을 지시합니다.
    if (AGM_Lobby* GM = Cast<AGM_Lobby>(GetWorld()->GetAuthGameMode()))
    {
        GM->MovePlayerToTeam(this, TargetTeamID);
    }
}

void APC_Lobby::Server_StartGame_Implementation()
{
    if (AGM_Lobby* GM = Cast<AGM_Lobby>(GetWorld()->GetAuthGameMode()))
    {
        GM->StartMobaMatch();
    }
}
