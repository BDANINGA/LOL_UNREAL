#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LOL_GameResultWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_GameResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULOL_GameResultWidget(const FObjectInitializer& ObjectInitializer);
	void SetVictory(bool bVictory);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY()
	TObjectPtr<class UTexture2D> VictoryTexture;
	UPROPERTY()
	TObjectPtr<class UTexture2D> DefeatTexture;
	UPROPERTY(Transient)
	TObjectPtr<class UImage> ResultImage;
};
