// 챔피언 UI
#include "Widget/LOL_ChampionWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"


void ULOL_ChampionWidget::UpdateHP(float Percent)
{
    if (HPbar) HPbar->SetPercent(Percent);
}

void ULOL_ChampionWidget::UpdateMP(float Percent)
{
    if (MPbar) MPbar->SetPercent(Percent);
}

void ULOL_ChampionWidget::AdjustLayoutForResolution()
{
    // HP바의 위치와 크기를 코드로 상세 조정
    if (UCanvasPanelSlot* HPSlot = Cast<UCanvasPanelSlot>(HPbar->Slot))
    {
        HPSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // 중앙 앵커
        HPSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        HPSlot->SetSize(FVector2D(200.f, 20.f));
    }

    // MP바를 HP바 바로 아래로 정밀 배치
    if (UCanvasPanelSlot* MPSlot = Cast<UCanvasPanelSlot>(MPbar->Slot))
    {
        MPSlot->SetPosition(FVector2D(0.f, 25.f));
    }
}