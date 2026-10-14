#include "Widget/LOL_DamageVignetteWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ULOL_DamageVignetteWidget::ULOL_DamageVignetteWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/UI/Effects/M_DamageVignette.M_DamageVignette"));
	if (Material.Succeeded()) VignetteMaterial = Material.Object;
}

void ULOL_DamageVignetteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(false);
	UImage* Glow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DamageEdgeGlow"));
	Glow->SetBrushFromMaterial(VignetteMaterial);
	Glow->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Glow;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Hidden);
}

void ULOL_DamageVignetteWidget::PlayHit(APawn* Victim, float DamageFraction)
{
	if (!VignetteMaterial || !GetWorld() || !Victim || DamageFraction <= 0.0f) return;
	// Refresh the pulse without stacking overlays or jumping back to transparent.
	StartOpacity = HitPawn == Victim ? GetRenderOpacity() : 0.0f;
	PeakOpacity = FMath::Max(StartOpacity, FMath::Clamp(0.30f + DamageFraction * 1.4f, 0.30f, 0.65f));
	HitPawn = Victim;
	StartedAt = GetWorld()->GetRealTimeSeconds();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULOL_DamageVignetteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const APlayerController* PC = GetOwningPlayer();
	const float Age = GetWorld() ? GetWorld()->GetRealTimeSeconds() - StartedAt : 1.0f;
	constexpr float Attack = 0.045f;
	constexpr float Duration = 0.60f;
	if (!PC || !HitPawn.IsValid() || PC->GetPawn() != HitPawn.Get() || Age >= Duration)
	{
		SetRenderOpacity(0.0f);
		SetVisibility(ESlateVisibility::Hidden);
		HitPawn.Reset();
		return;
	}
	const float Alpha = Age < Attack
		? FMath::Lerp(StartOpacity, PeakOpacity, FMath::SmoothStep(0.0f, Attack, Age))
		: PeakOpacity * (1.0f - FMath::SmoothStep(Attack, Duration, Age));
	SetRenderOpacity(Alpha);
}
