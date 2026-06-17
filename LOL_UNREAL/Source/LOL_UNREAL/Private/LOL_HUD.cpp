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
        MainHUDWidget->SetLevel(CurrentStat.Level);
    }
}

void ALOL_HUD::UpdateAll_Images(ABaseChampion* MyChamp)
{
    if (MyChamp->ChampionResource.Portrait_Circle) MainHUDWidget->Portrait_Image->SetBrushFromTexture(MyChamp->ChampionResource.Portrait_Circle);
    if (MyChamp->ChampionResource.SkillQ_Image) MainHUDWidget->SetSkillImage("Q", MyChamp->ChampionResource.SkillQ_Image);
    if (MyChamp->ChampionResource.SkillW_Image) MainHUDWidget->SetSkillImage("W", MyChamp->ChampionResource.SkillW_Image);
    if (MyChamp->ChampionResource.SkillE_Image) MainHUDWidget->SetSkillImage("E", MyChamp->ChampionResource.SkillE_Image);
    if (MyChamp->ChampionResource.SkillR_Image) MainHUDWidget->SetSkillImage("R", MyChamp->ChampionResource.SkillR_Image);
    if (MyChamp->ChampionResource.SkillP_Image) MainHUDWidget->SetSkillImage("P", MyChamp->ChampionResource.SkillP_Image);
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

void ALOL_HUD::UpdateGold(float NewGold)
{
    if (MainHUDWidget) MainHUDWidget->UpdateGold(NewGold);
}

void ALOL_HUD::UpdateEXP(float NewEXP)
{
    if (MyChampion == nullptr)
    {
        APlayerController* PC = GetOwningPlayerController();
        if (PC)
        {
            MyChampion = Cast<ABaseChampion>(PC->GetPawn());
        }
    }

    if (MainHUDWidget && MyChampion && MyChampion->StatComponent) MainHUDWidget->UpdateEXP(NewEXP, MyChampion->StatComponent->GetMaxEXP());
}

void ALOL_HUD::UpdateSkillCoolDown(FName SkillName, float CoolLocalEndTime, float CoolEndTime)
{
    if (MainHUDWidget)
        MainHUDWidget->SetSkillCooldown(SkillName, CoolLocalEndTime, CoolEndTime);
}
