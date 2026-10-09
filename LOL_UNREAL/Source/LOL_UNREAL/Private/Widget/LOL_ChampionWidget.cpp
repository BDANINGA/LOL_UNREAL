// 챔피언 UI
#include "Widget/LOL_ChampionWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULOL_ChampionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!Txt_Level && WidgetTree)
    {
        Txt_Level = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_Level")));
    }

    if (!EXPbar && WidgetTree)
    {
        const TArray<FName> EXPBarNames = {
            TEXT("EXPbar"),
            TEXT("EXPProgressBar"),
            TEXT("ExpProgressBar"),
            TEXT("ExperienceBar"),
            TEXT("EXP_BAR")
        };

        for (const FName& WidgetName : EXPBarNames)
        {
            EXPbar = Cast<UProgressBar>(WidgetTree->FindWidget(WidgetName));
            if (EXPbar)
            {
                break;
            }
        }
    }

    if (!experiencebar && WidgetTree)
    {
        experiencebar = Cast<UImage>(WidgetTree->FindWidget(TEXT("experiencebar")));
    }

    if (experiencebar)
    {
        EXP_MID = experiencebar->GetDynamicMaterial();
    }

    SetLevel(1);
    UpdateEXP(0.0f, 1.0f);
}

void ULOL_ChampionWidget::UpdateHP(float Percent)
{
    if (HPbar) HPbar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
}

void ULOL_ChampionWidget::UpdateMP(float Percent)
{
    if (MPbar)
    {
        MPbar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
    }
}

void ULOL_ChampionWidget::UpdateEXP(float NewEXP, float MaxEXP)
{
    const float Percent = MaxEXP > 0.0f
        ? FMath::Clamp(NewEXP / MaxEXP, 0.0f, 1.0f)
        : 0.0f;
    ApplyEXPPercent(Percent);
}

void ULOL_ChampionWidget::SetLevel(int32 Value)
{
    if (Txt_Level)
    {
        Txt_Level->SetText(FText::AsNumber(FMath::Clamp(Value, 1, 18)));
    }
}

UProgressBar* ULOL_ChampionWidget::GetEXPProgressBar() const
{
    return EXPbar;
}

void ULOL_ChampionWidget::ApplyEXPPercent(float Percent)
{
    CurrentEXPPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

    if (UProgressBar* EXPProgressBar = GetEXPProgressBar())
    {
        EXPProgressBar->SetPercent(CurrentEXPPercent);
    }

    if (EXP_MID)
    {
        EXP_MID->SetScalarParameterValue(TEXT("newexpratio"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Percent"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("percent"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Progress"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Ratio"), CurrentEXPPercent);
    }
}

void ULOL_ChampionWidget::AdjustLayoutForResolution()
{
    // HP바의 위치와 크기를 코드로 상세 조정
    if (UCanvasPanelSlot* HPSlot = HPbar ? Cast<UCanvasPanelSlot>(HPbar->Slot) : nullptr)
    {
        HPSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // 중앙 앵커
        HPSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        HPSlot->SetSize(FVector2D(200.f, 20.f));
    }

    // MP바를 HP바 바로 아래로 정밀 배치
    if (UCanvasPanelSlot* MPSlot = MPbar ? Cast<UCanvasPanelSlot>(MPbar->Slot) : nullptr)
    {
        MPSlot->SetPosition(FVector2D(0.f, 25.f));
    }
}
