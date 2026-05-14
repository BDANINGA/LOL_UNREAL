// HUD
#include "LOL_HUD.h"
#include "BaseChampion.h"
#include "Widget/LOL_HUDWidget.h"
#include "Component/LOL_StatComponent.h"

ALOL_HUD::ALOL_HUD()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> MainHUDAsset(TEXT("/Game/UI/HUD/Wbp_lol_hud.Wbp_lol_hud_C"));
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

void ALOL_HUD::UpdateStat(const FChampionStat& CurrentStat)
{
    if (MainHUDWidget) {
        MainHUDWidget->SetAttackDamage(CurrentStat.AttackDamage);
        MainHUDWidget->SetAbilityPower(CurrentStat.AbilityPower);
        MainHUDWidget->SetArmor(CurrentStat.Armor);
        MainHUDWidget->SetSpellBlock(CurrentStat.SpellBlock);
        MainHUDWidget->SetAttackSpeed(CurrentStat.AttackSpeed);
        MainHUDWidget->SetAbilityHaste(CurrentStat.AbilityHaste);
        MainHUDWidget->SetCriticalRate(CurrentStat.CriticalChance);
        MainHUDWidget->SetMoveSpeed(CurrentStat.MoveSpeed);
    }
}

void ALOL_HUD::UpdateAll_Images(ABaseChampion* MyChamp)
{
    if (MyChamp->Portrait_Circle) MainHUDWidget->Portrait_Image->SetBrushFromTexture(MyChamp->Portrait_Circle);
    if (MyChamp->SkillQ_Image) MainHUDWidget->SkillQ_Image->SetBrushFromTexture(MyChamp->SkillQ_Image);
    if (MyChamp->SkillW_Image) MainHUDWidget->SkillW_Image->SetBrushFromTexture(MyChamp->SkillW_Image);
    if (MyChamp->SkillE_Image) MainHUDWidget->SkillE_Image->SetBrushFromTexture(MyChamp->SkillE_Image);
    if (MyChamp->SkillR_Image) MainHUDWidget->SkillR_Image->SetBrushFromTexture(MyChamp->SkillR_Image);
    if (MyChamp->SkillP_Image) MainHUDWidget->SkillP_Image->SetBrushFromTexture(MyChamp->SkillP_Image);
}

void ALOL_HUD::UpdateHP(float NewHP)
{
    if (MyChampion == nullptr)
    {
        APlayerController* PC = GetOwningPlayerController();
        if (PC)
        {
            MyChampion = Cast<ABaseChampion>(PC->GetPawn());
        }
    }

    if (MainHUDWidget && MyChampion && MyChampion->StatComponent)
        MainHUDWidget->UpdateHP(NewHP, MyChampion->StatComponent->GetStat().MaxHP);
}
void ALOL_HUD::UpdateMP(float NewMP)
{
    if (MyChampion == nullptr)
    {
        APlayerController* PC = GetOwningPlayerController();
        if (PC)
        {
            MyChampion = Cast<ABaseChampion>(PC->GetPawn());
        }
    }

    if (MainHUDWidget && MyChampion && MyChampion->StatComponent) 
        MainHUDWidget->UpdateMP(NewMP, MyChampion->StatComponent->GetStat().MaxMP);
}
