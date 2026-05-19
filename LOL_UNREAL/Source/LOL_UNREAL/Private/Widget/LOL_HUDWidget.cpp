// HUD 관리하는 위젯 클래스
#include "Widget/LOL_HUDWidget.h"
#include "Components/ProgressBar.h"

void ULOL_HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SkillQ_Image) SkillQ_MID = SkillQ_Image->GetDynamicMaterial();
    if (SkillW_Image) SkillW_MID = SkillW_Image->GetDynamicMaterial();
    if (SkillE_Image) SkillE_MID = SkillE_Image->GetDynamicMaterial();
    if (SkillR_Image) SkillR_MID = SkillR_Image->GetDynamicMaterial();
}

void ULOL_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (SkillQ_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeQ - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeQ, 0.0f, 1.0f);
        }

        SkillQ_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillW_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeW - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeW, 0.0f, 1.0f);
        }

        SkillW_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillE_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeE - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeE, 0.0f, 1.0f);
        }

        SkillE_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillR_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeR - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeR, 0.0f, 1.0f);
        }

        SkillR_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
}

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

void ULOL_HUDWidget::SetSkillImage(FName SkillName, UTexture2D* IconTexture)
{
    UMaterialInstanceDynamic* TargetMID = nullptr;

    if (SkillName == TEXT("Q"))
    {
        TargetMID = SkillQ_MID;
    }
    else if (SkillName == TEXT("W"))
    {
        TargetMID = SkillW_MID;
    }
    else if (SkillName == TEXT("E"))
    {
        TargetMID = SkillE_MID;
    }
    else if (SkillName == TEXT("R"))
    {
        TargetMID = SkillR_MID;
    }
    if (TargetMID)
    {
        TargetMID->SetTextureParameterValue(TEXT("SkillIcon"), IconTexture);
    }
}

void ULOL_HUDWidget::SetSkillCooldown(FName SkillName, float CoolLocalEndTime, float CoolEndTime)
{
    if (SkillName == "Q")
    {
        SkillCoolLocalEndTimeQ = CoolLocalEndTime;
        SkillCoolEndTimeQ = CoolEndTime;
    }
    else if (SkillName == "W")
    {
        SkillCoolLocalEndTimeW = CoolLocalEndTime;
        SkillCoolEndTimeW = CoolEndTime;
    }
    else if (SkillName == "E")
    {
        SkillCoolLocalEndTimeE = CoolLocalEndTime;
        SkillCoolEndTimeE = CoolEndTime;
    }
    else if (SkillName == "R")
    {
        SkillCoolLocalEndTimeR = CoolLocalEndTime;
        SkillCoolEndTimeR = CoolEndTime;
    }
    else if (SkillName == "P")
    {
        SkillCoolLocalEndTimeP = CoolLocalEndTime;
        SkillCoolEndTimeP = CoolEndTime;
    }
}

