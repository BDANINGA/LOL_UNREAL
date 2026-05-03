// HUD
#include "Widget/LOL_HUD.h"
#include "Blueprint/UserWidget.h"

ALOL_HUD::ALOL_HUD()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> MainHUDAsset(TEXT("/Game/UI/Wbp_lol_hud.Wbp_lol_hud_C"));
    if (MainHUDAsset.Succeeded())
    {
        MainHUDClass = MainHUDAsset.Class;
    }
}

void ALOL_HUD::BeginPlay()
{
    Super::BeginPlay();

    if (MainHUDClass)
    {
        MainHUDWidget = CreateWidget<UUserWidget>(GetWorld(), MainHUDClass);
        if (MainHUDWidget)
        {
            MainHUDWidget->AddToViewport();
        }
    }
}