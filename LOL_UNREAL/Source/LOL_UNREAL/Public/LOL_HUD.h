// HUD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/TextBlock.h"
#include "LOL_HUD.generated.h"

class ULOL_HUDWidget;
class ABaseChampion;

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

    void UpdateAttackDamage(float CurrentAttackDamage);
    void UpdateAbilityPower(float CurrentAbilityPower);
    void UpdateArmor(float CurrentArmor);
    void UpdateSpellBlock(float CurrentSpellBlock);
    void UpdateAttackSpeed(float CurrentAttackSpeed);
    void UpdateAbilityHaste(float CurrentAbilityHaste);
    void UpdateCriticalRate(float CurrentCriticalRate);
    void UpdateMoveSpeed(float CurrentMoveSpeed);

    void UpdateAll_Images(ABaseChampion* MYChamp);

protected:
    virtual void BeginPlay() override;
};
