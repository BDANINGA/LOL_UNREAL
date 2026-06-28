#include "Lobby/GM_Lobby.h"
#include "Lobby/PS_Lobby.h"
#include "Lobby/PC_Lobby.h"
#include "Lobby/GS_Lobby.h"
#include "Lobby/LOL_GameInstance.h"

AGM_Lobby::AGM_Lobby()
{
    PlayerControllerClass = APC_Lobby::StaticClass();
    PlayerStateClass = APS_Lobby::StaticClass();
    GameStateClass = AGS_Lobby::StaticClass();
    bUseSeamlessTravel = true;
}

void AGM_Lobby::OnPostLogin(AController* NewPlayer)
{
    Super::OnPostLogin(NewPlayer);

    APlayerController* PC = Cast<APlayerController>(NewPlayer);
    if (!PC) return;

    APS_Lobby* PS = NewPlayer->GetPlayerState<APS_Lobby>();
    if (!PS) return;

    // 지그재그 자동 배정: 왼쪽 팀이 오른쪽보다 작거나 같으면 왼쪽으로
    if (LeftTeam.Num() <= RightTeam.Num())
    {
        LeftTeam.Add(PC);
        PS->TeamID = 1;
    }
    else
    {
        RightTeam.Add(PC);
        PS->TeamID = 2;
    }

    // The listen-server host owns the server GameInstance. Initialize its
    // nickname here so widget/PlayerController BeginPlay order cannot erase it.
    if (PC->IsLocalController())
    {
        if (ULOL_GameInstance* GameInstance = PC->GetGameInstance<ULOL_GameInstance>())
        {
            FString SavedNickname = GameInstance->MySavedNickname;
            SavedNickname.TrimStartAndEndInline();
            if (!SavedNickname.IsEmpty())
            {
                PS->Nickname = SavedNickname.Left(20);
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("Lobby host nickname initialized in PostLogin. Nickname=%s TeamID=%d"),
                    *PS->Nickname,
                    PS->TeamID);
            }
        }
    }

    PS->SyncLocalGameInstance();
    PS->ForceNetUpdate();

    if (AGS_Lobby* GS = GetGameState<AGS_Lobby>())
    {
        GS->NotifyTeamChanged();
    }
}

void AGM_Lobby::MovePlayerToTeam(APlayerController* Player, uint8 TargetTeamID)
{
    APS_Lobby* PS = Player->GetPlayerState<APS_Lobby>();
    if (!PS || PS->TeamID == TargetTeamID) return;

    // 이동하려는 팀의 정원이 5명 미만인지 확인
    if (TargetTeamID == 1 && LeftTeam.Num() < 5)
    {
        RightTeam.Remove(Player);
        LeftTeam.Add(Player);
        PS->TeamID = 1;
    }
    else if (TargetTeamID == 2 && RightTeam.Num() < 5)
    {
        LeftTeam.Remove(Player);
        RightTeam.Add(Player);
        PS->TeamID = 2;
    }
    PS->SyncLocalGameInstance();

    if (AGS_Lobby* GS = GetGameState<AGS_Lobby>())
    {
        GS->NotifyTeamChanged();
    }
}

void AGM_Lobby::StartMobaMatch()
{
    // 게임 시작 신호가 떨어지면 즉시 로비 진입을 잠급니다.
    bLobbyLocked = true;

    FString MapPath = TEXT("/Game/ChampionSelectMap/ChampionSelectMap?listen?game=/Game/ChampionSelectMap/MyGM_ChampSelect.MyGM_ChampSelect_C");
    // 픽창 레벨로 모든 플레이어 이동
    GetWorld()->ServerTravel(MapPath, true);
}

void AGM_Lobby::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

    if (bLobbyLocked)
    {
        ErrorMessage = TEXT("Lobby is closing. Match starting...");
    }
}
