#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LOL_QuitConfirmWidget.generated.h"

// Built in C++ so every in-game controller has the dialog without Blueprint setup.
UCLASS()
class LOL_UNREAL_API ULOL_QuitConfirmWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FocusCancelButton();

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

private:
	UFUNCTION()
	void HandleYes();

	UFUNCTION()
	void HandleNo();

	UPROPERTY(Transient)
	class UButton* NoButton = nullptr;
};
