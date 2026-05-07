// 챔피언 UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

public:
    void UpdateHP(float Percent);
    void UpdateMP(float Percent);

    void AdjustLayoutForResolution();
};
