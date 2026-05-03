// 마우스 커서
#include "Widget/LOL_CursorWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/Image.h"
#include "PaperSprite.h"

void ULOL_CursorWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (CursorTable.Contains(TEXT("Normal")))
    {
        SwitchCursorState(TEXT("Normal"));
    }
}
void ULOL_CursorWidget::SwitchCursorState(FString StateName)
{
    if (CursorImage && CursorTable.Contains(StateName))
    {
        UPaperSprite* SelectedSprite = CursorTable[StateName];
        if (SelectedSprite)
        {
            CursorImage->SetBrushFromAtlasInterface(SelectedSprite);
        }
    }
}