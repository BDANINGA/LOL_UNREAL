// HUD 관리하는 위젯 클래스
#include "Widget/LOL_HUDWidget.h"
#include "Components/ProgressBar.h"


void ULOL_HUDWidget::UpdateHP(float NewHP, float MaxHP)
{
	if (HPProgressBar) HPProgressBar->SetPercent(NewHP / MaxHP);

    if (Txt_HP)
    {
        FText HPText = FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(FMath::FloorToInt(NewHP)),
            FText::AsNumber(FMath::FloorToInt(MaxHP))
        );

        Txt_HP->SetText(HPText);
    }
}
void ULOL_HUDWidget::UpdateMP(float NewMP, float MaxMP)
{
	if (MPProgressBar) MPProgressBar->SetPercent(NewMP / MaxMP);

    if (Txt_MP)
    {
        FText MPText = FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(FMath::FloorToInt(NewMP)),
            FText::AsNumber(FMath::FloorToInt(MaxMP))
        );

        Txt_MP->SetText(MPText);
    }
}