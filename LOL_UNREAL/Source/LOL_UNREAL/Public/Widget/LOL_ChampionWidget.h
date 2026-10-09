// 챔피언 UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "LOL_ChampionWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_ChampionWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HPbar;

    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* MPbar;

    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* EXPbar;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* experiencebar;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Txt_Level;

    UPROPERTY()
    class UMaterialInstanceDynamic* EXP_MID;

    UPROPERTY()
    float CurrentEXPPercent = 0.0f;

    virtual void NativeConstruct() override;

public:
    void UpdateHP(float Percent);
    void UpdateMP(float Percent);
    void UpdateEXP(float NewEXP, float MaxEXP);

    void SetLevel(int32 Value);

    void AdjustLayoutForResolution();

private:
    class UProgressBar* GetEXPProgressBar() const;
    void ApplyEXPPercent(float Percent);
};
