// 마우스 커서
#include "Widget/LOL_CursorWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "PaperSprite.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"

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
            if (StateName != "Normal" && StateName != "NormalFriendly") {
                CursorImage->SetRenderScale(FVector2D(0.5f, 0.5f));
                CursorImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            }
            else {
                CursorImage->SetRenderScale(FVector2D(0.3f, 0.3f));
                CursorImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            }
        }
    }
}