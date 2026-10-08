#include "Widget/LOL_QuitConfirmWidget.h"

#include "LOL_PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"

void ULOL_QuitConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);

	const FLinearColor Gold(0.55f, 0.39f, 0.16f);
	const FLinearColor Cream(0.91f, 0.85f, 0.68f);
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.0f, 0.005f, 0.012f, 0.72f));
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Backdrop;

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(480.f);
	Backdrop->SetContent(Size);
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(Gold);
	Frame->SetPadding(FMargin(2.f));
	Size->SetContent(Frame);
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.009f, 0.021f, 0.032f));
	Panel->SetPadding(FMargin(36.f, 28.f));
	Frame->SetContent(Panel);
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Content);

	auto AddText = [&](FName Name, const FText& Text, int32 FontSize, FLinearColor Color, FMargin SlotPadding)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FontSize));
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(ETextJustify::Center);
		Content->AddChildToVerticalBox(Label)->SetPadding(SlotPadding);
	};
	AddText(TEXT("Title"), NSLOCTEXT("QuitDialog", "Title", "게임 종료"), 16, Gold, FMargin(0.f, 0.f, 0.f, 20.f));
	AddText(TEXT("Question"), NSLOCTEXT("QuitDialog", "Question", "정말 종료하시겠습니까?"), 24, Cream, FMargin(0.f, 0.f, 0.f, 30.f));

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Buttons);
	auto AddButton = [&](FName Name, const FText& Text, FMargin SlotPadding)
	{
		USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonSize->SetHeightOverride(48.f);
		UHorizontalBoxSlot* Slot = Buttons->AddChildToHorizontalBox(ButtonSize);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(SlotPadding);
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		auto Brush = [&](FLinearColor Fill, FLinearColor Outline)
		{
			FSlateBrush Result;
			Result.DrawAs = ESlateBrushDrawType::RoundedBox;
			Result.TintColor = FSlateColor(Fill);
			Result.OutlineSettings = FSlateBrushOutlineSettings(0.f, Outline, 1.f);
			return Result;
		};
		Style.SetNormal(Brush(FLinearColor(0.018f, 0.042f, 0.058f), Gold));
		Style.SetHovered(Brush(FLinearColor(0.025f, 0.12f, 0.15f), Cream));
		Style.SetPressed(Brush(FLinearColor(0.009f, 0.028f, 0.04f), Gold));
		Button->SetStyle(Style);
		ButtonSize->SetContent(Button);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
		Label->SetColorAndOpacity(FSlateColor(Cream));
		Button->SetContent(Label);
		return Button;
	};
	UButton* YesButton = AddButton(TEXT("YesButton"), NSLOCTEXT("QuitDialog", "Yes", "Yes"), FMargin(0.f, 0.f, 8.f, 0.f));
	NoButton = AddButton(TEXT("NoButton"), NSLOCTEXT("QuitDialog", "No", "No"), FMargin(8.f, 0.f, 0.f, 0.f));
	for (EUINavigation Direction : {EUINavigation::Next, EUINavigation::Previous, EUINavigation::Left, EUINavigation::Right, EUINavigation::Up, EUINavigation::Down})
	{
		YesButton->SetNavigationRuleExplicit(Direction, NoButton);
		NoButton->SetNavigationRuleExplicit(Direction, YesButton);
	}
	YesButton->OnClicked.AddDynamic(this, &ULOL_QuitConfirmWidget::HandleYes);
	NoButton->OnClicked.AddDynamic(this, &ULOL_QuitConfirmWidget::HandleNo);
	AddText(TEXT("Hint"), NSLOCTEXT("QuitDialog", "Hint", "ESC · 게임으로 돌아가기"), 12, FLinearColor(0.3f, 0.39f, 0.42f), FMargin(0.f, 20.f, 0.f, 0.f));
}

void ULOL_QuitConfirmWidget::FocusCancelButton()
{
	if (NoButton)
	{
		NoButton->SetUserFocus(GetOwningPlayer());
	}
}

FReply ULOL_QuitConfirmWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		HandleNo();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FReply ULOL_QuitConfirmWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	// Keep background clicks from changing viewport focus or reaching the HUD.
	FocusCancelButton();
	return FReply::Handled();
}

void ULOL_QuitConfirmWidget::HandleYes()
{
	if (ALOL_PlayerController* Controller = Cast<ALOL_PlayerController>(GetOwningPlayer()))
	{
		Controller->ConfirmQuitGame();
	}
}

void ULOL_QuitConfirmWidget::HandleNo()
{
	if (ALOL_PlayerController* Controller = Cast<ALOL_PlayerController>(GetOwningPlayer()))
	{
		Controller->CancelQuitGame();
	}
}
