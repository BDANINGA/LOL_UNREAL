// 마우스 커서
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "PaperSprite.h"
#include "LOL_CursorWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_CursorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
    TMap<FString, UPaperSprite*> CursorTable;

    UPROPERTY(meta = (BindWidget))
    UImage* CursorImage;    

    void NativeConstruct();
    void SwitchCursorState(FString StateName);

};
