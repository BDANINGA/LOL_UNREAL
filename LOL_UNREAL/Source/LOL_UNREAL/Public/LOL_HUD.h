// HUD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/TextBlock.h"
#include "LOL_HUD.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_HUD : public AHUD
{
	GENERATED_BODY()
	
public:
    ALOL_HUD();
    
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> MainHUDClass;

    UPROPERTY()
    UUserWidget* MainHUDWidget;

    void UpdateAD(float CurrentAD);
    void UpdateAP(float CurrentAP);
    void UpdateAR(float CurrentAR);
    void UpdateMR(float CurrentMR);
    void UpdateAS(float CurrentAS);
    void UpdateCD(float CurrentCD);
    void UpdateCR(float CurrentCR);
    void UpdateSP(float CurrentSP);

protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ad_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ap_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* armor_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* magic_resis_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* attack_sp_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* cooldown_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* critical_rate_tex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* speed_tex;
};
