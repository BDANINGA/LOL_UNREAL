// 챔피언 UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "LOL_ChampionWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_ChampionWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HPbar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* MPbar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Level;

public:
    void UpdateHP(float Percent);
    void UpdateMP(float Percent);

    void SetLevel(float Value) { if (Txt_Level) Txt_Level->SetText(FText::AsNumber(Value)); }

    void AdjustLayoutForResolution();
};
