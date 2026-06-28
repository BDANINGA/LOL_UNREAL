#include "ChampionSelect/PC_ChampSelect.h"
#include "ChampionSelect/PS_ChampSelect.h"
#include "ChampionSelect/GM_ChampSelect.h"
#include "Blueprint/UserWidget.h"

void APC_ChampSelect::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() && ChampSelectWidgetClass)
    {
        ChampSelectWidget = CreateWidget<UUserWidget>(this, ChampSelectWidgetClass);
        if (ChampSelectWidget)
        {
            ChampSelectWidget->AddToViewport();
            bShowMouseCursor = true;
            SetInputMode(FInputModeUIOnly());
        }
    }
}

void APC_ChampSelect::Server_HoverChampion_Implementation(EChampionID Champ)
{
    APS_ChampSelect* PS = GetPlayerState<APS_ChampSelect>();
    if (PS && !PS->bIsReady)
    {
        PS->HoveredChampion = Champ;
    }
}

void APC_ChampSelect::Server_LockInChampion_Implementation()
{
    APS_ChampSelect* PS = GetPlayerState<APS_ChampSelect>();
    if (PS && !PS->bIsReady && PS->HoveredChampion != EChampionID::None)
    {
        PS->LockedChampion = PS->HoveredChampion;
        PS->bIsReady = true;
        PS->SyncLocalGameInstance();

        AGM_ChampSelect* GM = Cast<AGM_ChampSelect>(GetWorld()->GetAuthGameMode());
        if (GM)
        {
            GM->CheckAllPlayersReady();
        }
    }
}
