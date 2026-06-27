// HUD 관리하는 위젯 클래스
#include "Widget/LOL_HUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

void ULOL_HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SkillQ_Image) SkillQ_MID = SkillQ_Image->GetDynamicMaterial();
    if (SkillW_Image) SkillW_MID = SkillW_Image->GetDynamicMaterial();
    if (SkillE_Image) SkillE_MID = SkillE_Image->GetDynamicMaterial();
    if (SkillR_Image) SkillR_MID = SkillR_Image->GetDynamicMaterial();

    CacheItemSlotImages();
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

void ULOL_HUDWidget::UpdateEXP(float NewEXP, float MaxEXP)
{
    /*if (EXPProgressBar) MPProgressBar->SetPercent(NewEXP / MaxEXP);*/
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

void ULOL_HUDWidget::AddItemIcon(UTexture2D* IconTexture)
{
    if (!IconTexture)
    {
        return;
    }

    if (CachedItemSlotImages.Num() == 0)
    {
        CacheItemSlotImages();
    }

    if (!CachedItemSlotImages.IsValidIndex(NextItemSlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD item slot is full or not found. SlotIndex=%d SlotCount=%d"), NextItemSlotIndex, CachedItemSlotImages.Num());
        return;
    }

    UImage* SlotImage = CachedItemSlotImages[NextItemSlotIndex];
    if (!SlotImage)
    {
        return;
    }

    FVector2D SlotPosition = FVector2D::ZeroVector;
    FVector2D SlotSize = FVector2D::ZeroVector;
    if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotImage->Slot))
    {
        SlotPosition = CanvasSlot->GetPosition();
        SlotSize = CanvasSlot->GetSize();
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("HUD item icon set. SlotIndex=%d Widget=%s CanvasPos=(%.1f, %.1f) CanvasSize=(%.1f, %.1f)"),
        NextItemSlotIndex,
        *SlotImage->GetName(),
        SlotPosition.X,
        SlotPosition.Y,
        SlotSize.X,
        SlotSize.Y
    );

    SlotImage->SetColorAndOpacity(FLinearColor::White);
    SlotImage->SetBrushFromTexture(IconTexture, false);

    FSlateBrush Brush = SlotImage->GetBrush();
    Brush.ImageSize = SlotSize.X > 0.f && SlotSize.Y > 0.f ? SlotSize : FVector2D(26.f, 26.f);
    SlotImage->SetBrush(Brush);
    ++NextItemSlotIndex;
}

void ULOL_HUDWidget::SetItemIcons(const TArray<UTexture2D*>& IconTextures)
{
    if (CachedItemSlotImages.Num() == 0)
    {
        CacheItemSlotImages();
    }

    NextItemSlotIndex = 0;

    for (UImage* SlotImage : CachedItemSlotImages)
    {
        if (SlotImage)
        {
            SlotImage->SetBrushFromTexture(nullptr);
            SlotImage->SetColorAndOpacity(FLinearColor::Transparent);
        }
    }

    for (UTexture2D* IconTexture : IconTextures)
    {
        AddItemIcon(IconTexture);
    }
}

void ULOL_HUDWidget::CacheItemSlotImages()
{
    CachedItemSlotImages.Reset();

    if (!WidgetTree)
    {
        return;
    }

    auto AddItemSlotIfValid = [this](UImage* SlotImage, const TCHAR* ExpectedName)
    {
        if (!SlotImage)
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD item slot missing. Expected=%s"), ExpectedName);
            return;
        }

        if (SlotImage == SkillQ_Image || SlotImage == SkillW_Image || SlotImage == SkillE_Image || SlotImage == SkillR_Image || SlotImage == SkillP_Image)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("HUD item slot rejected because it is a skill image. Expected=%s ActualWidget=%s"),
                ExpectedName,
                *SlotImage->GetName()
            );
            return;
        }

        CachedItemSlotImages.Add(SlotImage);
        UE_LOG(LogTemp, Log, TEXT("HUD item slot accepted. Expected=%s ActualWidget=%s"), ExpectedName, *SlotImage->GetName());
    };

    AddItemSlotIfValid(ItemSlot_1, TEXT("ItemSlot_1"));
    AddItemSlotIfValid(ItemSlot_2, TEXT("ItemSlot_2"));
    AddItemSlotIfValid(ItemSlot_3, TEXT("ItemSlot_3"));
    AddItemSlotIfValid(ItemSlot_4, TEXT("ItemSlot_4"));
    AddItemSlotIfValid(ItemSlot_5, TEXT("ItemSlot_5"));
    AddItemSlotIfValid(ItemSlot_6, TEXT("ItemSlot_6"));

    if (CachedItemSlotImages.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("HUD item slots cached by explicit bind. Count=%d"), CachedItemSlotImages.Num());
        return;
    }

    const TArray<FName> PreferredSlotNames =
    {
        FName("ItemSlot_1"),
        FName("ItemSlot_2"),
        FName("ItemSlot_3"),
        FName("ItemSlot_4"),
        FName("ItemSlot_5"),
        FName("ItemSlot_6")
    };

    for (const FName& SlotName : PreferredSlotNames)
    {
        if (UImage* SlotImage = Cast<UImage>(WidgetTree->FindWidget(SlotName)))
        {
            AddItemSlotIfValid(SlotImage, *SlotName.ToString());
        }
    }

    if (CachedItemSlotImages.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("HUD item slots cached by name. Count=%d"), CachedItemSlotImages.Num());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HUD item slots not found. Rename the six small item icon Images to ItemSlot_1 through ItemSlot_6."));
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

