// HUD
#include "LOL_HUD.h"
#include "BaseChampion.h"
#include "Widget/LOL_HUDWidget.h"

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

    APlayerController* PC = GetOwningPlayerController();
    if (PC && PC->IsLocalController())
    {
        if (MainHUDWidget)
        {
            MainHUDWidget->RemoveFromParent();
            MainHUDWidget = nullptr;
        }

        // 위젯 생성
        if (MainHUDClass)
        {
            MainHUDWidget = CreateWidget<ULOL_HUDWidget>(PC, MainHUDClass);
            if (MainHUDWidget)
            {
                MainHUDWidget->AddToViewport();
            }
        }
    }
}

void ALOL_HUD::UpdateAttackDamage(float CurrentAttackDamage)
{
    if (MainHUDWidget) MainHUDWidget->SetAttackDamage(CurrentAttackDamage);
}
void ALOL_HUD::UpdateAbilityPower(float CurrentAbilityPower)
{
    if (MainHUDWidget) MainHUDWidget->SetAbilityPower(CurrentAbilityPower);
}
void ALOL_HUD::UpdateArmor(float CurrentArmor)
{
    if (MainHUDWidget) MainHUDWidget->SetArmor(CurrentArmor);
}

void ALOL_HUD::UpdateSpellBlock(float CurrentSpellBlock)
{
    if (MainHUDWidget) MainHUDWidget->SetSpellBlock(CurrentSpellBlock);
}

void ALOL_HUD::UpdateAttackSpeed(float CurrentAttackSpeed)
{
    if (MainHUDWidget) MainHUDWidget->SetAttackSpeed(CurrentAttackSpeed);
}
void ALOL_HUD::UpdateAbilityHaste(float CurrentAbilityHaste)
{
    if (MainHUDWidget) MainHUDWidget->SetAbilityHaste(CurrentAbilityHaste);
}
void ALOL_HUD::UpdateCriticalRate(float CurrentCriticalRate)
{
    if (MainHUDWidget) MainHUDWidget->SetCriticalRate(CurrentCriticalRate);
}
void ALOL_HUD::UpdateMoveSpeed(float CurrentMoveSpeed)
{
    if (MainHUDWidget) MainHUDWidget->SetMoveSpeed(CurrentMoveSpeed);
}

void ALOL_HUD::UpdateAll_Images(ABaseChampion* MyChamp)
{
    if (MyChamp->Portrait_Image) MainHUDWidget->Portrait_Image->SetBrushFromTexture(MyChamp->Portrait_Image);
    if (MyChamp->SkillQ_Image) MainHUDWidget->SkillQ_Image->SetBrushFromTexture(MyChamp->SkillQ_Image);
    if (MyChamp->SkillW_Image) MainHUDWidget->SkillW_Image->SetBrushFromTexture(MyChamp->SkillW_Image);
    if (MyChamp->SkillE_Image) MainHUDWidget->SkillE_Image->SetBrushFromTexture(MyChamp->SkillE_Image);
    if (MyChamp->SkillR_Image) MainHUDWidget->SkillR_Image->SetBrushFromTexture(MyChamp->SkillR_Image);
    if (MyChamp->SkillP_Image) MainHUDWidget->SkillP_Image->SetBrushFromTexture(MyChamp->SkillP_Image);
}
