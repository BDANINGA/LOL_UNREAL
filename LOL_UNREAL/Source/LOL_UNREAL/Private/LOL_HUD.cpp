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

void ALOL_HUD::UpdateAD(float CurrentAD)
{
    if (ad_tex)
        ad_tex->SetText(FText::AsNumber(CurrentAD));
}
void ALOL_HUD::UpdateAP(float CurrentAP)
{
    if (ap_tex)
        ap_tex->SetText(FText::AsNumber(CurrentAP));
}
void ALOL_HUD::UpdateAR(float CurrentAR)
{
    if (armor_tex)
        armor_tex->SetText(FText::AsNumber(CurrentAR));
}

void ALOL_HUD::UpdateMR(float CurrentMR)
{
    if (magic_resis_tex)
        magic_resis_tex->SetText(FText::AsNumber(CurrentMR));
}

void ALOL_HUD::UpdateAS(float CurrentAS)
{
    if (attack_sp_tex)
        attack_sp_tex->SetText(FText::AsNumber(CurrentAS));
}
void ALOL_HUD::UpdateCD(float CurrentCD)
{
    if (cooldown_tex)
        cooldown_tex->SetText(FText::AsNumber(CurrentCD));
}
void ALOL_HUD::UpdateCR(float CurrentCR)
{
    if (critical_rate_tex)
        critical_rate_tex->SetText(FText::AsNumber(CurrentCR));
}
void ALOL_HUD::UpdateSP(float CurrentSP)
{
    if (speed_tex)
        speed_tex->SetText(FText::AsNumber(CurrentSP));
}
