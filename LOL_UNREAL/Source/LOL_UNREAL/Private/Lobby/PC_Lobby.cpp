#include "Lobby/PC_Lobby.h"
#include "Lobby/PS_Lobby.h"
#include "Lobby/GM_Lobby.h"
#include "Lobby/GS_Lobby.h"
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
            GI->CompleteLobbyJoin();
            UE_LOG(
                LogTemp,
                Log,
                TEXT("Submitting saved lobby nickname. Nickname=%s LocalRole=%d"),
                *GI->MySavedNickname,
                static_cast<int32>(GetLocalRole()));
            Server_SetNickname(GI->MySavedNickname);
        }
    }
}

void APC_Lobby::Server_SetNickname_Implementation(const FString& NewNickname)
{
    if (APS_Lobby* PS = GetPlayerState<APS_Lobby>())
    {
        FString SanitizedNickname = NewNickname;
        SanitizedNickname.TrimStartAndEndInline();
        if (SanitizedNickname.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("Lobby nickname was empty. Player=%s"), *GetNameSafe(this));
            return;
        }

        PS->Nickname = SanitizedNickname.Left(20);
        PS->SyncLocalGameInstance();

        // RepNotify does not execute on the listen-server authority copy.
        PS->OnPlayerDataReplicated.Broadcast();
        PS->ForceNetUpdate();

        if (AGS_Lobby* GS = GetWorld()->GetGameState<AGS_Lobby>())
        {
            GS->NotifyTeamChanged();
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("Lobby nickname registered. Player=%s Nickname=%s TeamID=%d"),
            *GetNameSafe(this),
            *PS->Nickname,
            PS->TeamID);
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
