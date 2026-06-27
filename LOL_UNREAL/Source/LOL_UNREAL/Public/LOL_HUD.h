// HUD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/TextBlock.h"
#include "LOL_HUD.generated.h"

class ULOL_HUDWidget;
class ABaseChampion;
class UTexture2D;

UCLASS()
class LOL_UNREAL_API ALOL_HUD : public AHUD
{
	GENERATED_BODY()
	
public:
    ALOL_HUD();
    
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<ULOL_HUDWidget> MainHUDClass;

    UPROPERTY()
    ULOL_HUDWidget* MainHUDWidget;

    void UpdateStat(const struct FChampionStat& CurrentStat);

    void UpdateAll_Images(ABaseChampion* MYChamp);

    void UpdateHP(float NewHP);
    void UpdateMP(float NewMP);
    void UpdateGold(float NewGold);
    void UpdateEXP(float NewEXP);

    void UpdateSkillCoolDown(FName SkillName, float CoolLocalEndTime, float CoolEndTime);
    void AddItemIcon(UTexture2D* IconTexture);
    void SetItemIcons(const TArray<UTexture2D*>& IconTextures);

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    ABaseChampion* MyChampion;
};
