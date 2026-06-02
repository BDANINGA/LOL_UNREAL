// 미니언 UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LOL_MinionWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_MinionWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HPbar;

public:
	void UpdateHP(float Percent);
};
