// 미니언 UI
#include "Widget/LOL_MinionWidget.h"
#include "Components/ProgressBar.h"

void ULOL_MinionWidget::UpdateHP(float Percent)
{
    if (HPbar)
    {
        HPbar->SetPercent(Percent);
    }
}
void ULOL_MinionWidget::SetHPBarImage(UTexture2D* FillTexture)
{
    if (HPbar && FillTexture)
    {
        FProgressBarStyle Style = HPbar->GetWidgetStyle();

        Style.FillImage.SetResourceObject(FillTexture);

        FLinearColor BgColor = FLinearColor(0.02f, 0.02f, 0.02f, 0.8f);
        Style.BackgroundImage.TintColor = FSlateColor(BgColor);

        HPbar->SetWidgetStyle(Style);
    }
}

