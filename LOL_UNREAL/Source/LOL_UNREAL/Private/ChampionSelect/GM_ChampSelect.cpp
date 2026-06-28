#include "ChampionSelect/GM_ChampSelect.h"
#include "ChampionSelect/GS_ChampSelect.h"
#include "ChampionSelect/PS_ChampSelect.h"
#include "ChampionSelect/PC_ChampSelect.h"

AGM_ChampSelect::AGM_ChampSelect()
{
    GameStateClass = AGS_ChampSelect::StaticClass();
    PlayerControllerClass = APC_ChampSelect::StaticClass();
    PlayerStateClass = APS_ChampSelect::StaticClass();
    bUseSeamlessTravel = true;
}

void AGM_ChampSelect::BeginPlay()
{
    Super::BeginPlay();

    // 1초마다 반복되는 타이머 시작
    GetWorldTimerManager().SetTimer(TimerHandle_MatchCountdown, this, &AGM_ChampSelect::AdvanceTimer, 1.0f, true);
}

void AGM_ChampSelect::AdvanceTimer()
{
    AGS_ChampSelect* GS = GetGameState<AGS_ChampSelect>();
    if (!GS) return;

    GS->TimeLeft--;

    GS->OnTimerChanged.Broadcast(GS->TimeLeft);

    // 시간 종료 시 또는 모두 준비되었을 때 본 게임으로 강제 이동
    if (GS->TimeLeft <= 0)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_MatchCountdown);
        StartInGameMatch();
    }
}

void AGM_ChampSelect::CheckAllPlayersReady()
{
    AGS_ChampSelect* GS = GetGameState<AGS_ChampSelect>();
    if (!GS) return;

    bool bAllReady = true;
    for (APlayerState* PS : GS->PlayerArray)
    {
        APS_ChampSelect* ChampPS = Cast<APS_ChampSelect>(PS);
        if (ChampPS && !ChampPS->bIsReady)
        {
            bAllReady = false;
            break;
        }
    }

    if (bAllReady)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_MatchCountdown);
        StartInGameMatch();
    }
}

void AGM_ChampSelect::StartInGameMatch()
{
    if (bTravelStarted)
    {
        return;
    }
    bTravelStarted = true;

    AGS_ChampSelect* GS = GetGameState<AGS_ChampSelect>();
    if (GS)
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            APS_ChampSelect* ChampPS = Cast<APS_ChampSelect>(PS);
            if (ChampPS && !ChampPS->bIsReady)
            {
                ChampPS->LockedChampion = EChampionID::Garen;
                ChampPS->bIsReady = true;
                ChampPS->SyncLocalGameInstance();
            }
        }
    }

    const FString GameMapURL =
        TEXT("/Game/Level/LOL_Map?listen?game=/Game/Level/MyLOL_GameModeBase.MyLOL_GameModeBase_C");

    UE_LOG(LogTemp, Warning, TEXT("Champion select traveling to: %s"), *GameMapURL);
    GetWorld()->ServerTravel(GameMapURL, true);
}

void AGM_ChampSelect::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

    ErrorMessage = TEXT("Game has already started.");
}
