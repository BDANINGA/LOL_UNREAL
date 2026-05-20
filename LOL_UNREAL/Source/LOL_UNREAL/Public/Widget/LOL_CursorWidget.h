// 마우스 커서
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LOL_CursorWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_CursorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
    TMap<FString, class UPaperSprite*> CursorTable;

    virtual void NativeConstruct() override;

    void SwitchCursorState(FString StateName);

private:
    UPROPERTY(meta = (BindWidget))
    class UImage* CursorImage;

};
