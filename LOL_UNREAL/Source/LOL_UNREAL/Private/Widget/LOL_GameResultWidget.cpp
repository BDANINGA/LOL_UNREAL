#include "Widget/LOL_GameResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ULOL_GameResultWidget::ULOL_GameResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Victory(TEXT("/Game/UI/victory_defeat/victory.victory"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Defeat(TEXT("/Game/UI/victory_defeat/defeat.defeat"));
	VictoryTexture = Victory.Object;
	DefeatTexture = Defeat.Object;
}

void ULOL_GameResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Presentation only; existing Escape/quit controls remain available.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit);
	UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Scale);
	ImageSlot->SetAnchors(FAnchors(0.18f, 0.15f, 0.82f, 0.85f));
	ImageSlot->SetOffsets(FMargin(0.f));
	ResultImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ResultImage"));
	Scale->SetContent(ResultImage);
}

void ULOL_GameResultWidget::SetVictory(bool bVictory)
{
	if (ResultImage)
	{
		ResultImage->SetBrushFromTexture(bVictory ? VictoryTexture : DefeatTexture, true);
	}
}
