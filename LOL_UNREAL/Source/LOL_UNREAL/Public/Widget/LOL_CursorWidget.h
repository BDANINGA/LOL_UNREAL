// 마우스 커서
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "LOL_CursorWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_CursorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    ULOL_CursorWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TMap<FString, UTexture2D*> CursorTable;

    UPROPERTY(meta = (BindWidget))
    UImage* CursorImage;    

    void NativeConstruct();
    void SwitchCursorState(FString StateName);

};
