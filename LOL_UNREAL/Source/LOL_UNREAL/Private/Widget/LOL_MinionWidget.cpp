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

