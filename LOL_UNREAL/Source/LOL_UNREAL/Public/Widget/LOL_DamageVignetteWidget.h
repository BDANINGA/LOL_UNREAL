#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LOL_DamageVignetteWidget.generated.h"

// A local, non-interactive overlay. One instance is reused for successive hits.
UCLASS()
class LOL_UNREAL_API ULOL_DamageVignetteWidget : public UUserWidget
{

	GENERATED_BODY()
public:
	ULOL_DamageVignetteWidget(const FObjectInitializer& ObjectInitializer);
	void PlayHit(APawn* Victim, float DamageFraction);

	UFUNCTION(BlueprintPure, Category = "UI|Damage Feedback")
	float GetFlashOpacity() const { return GetRenderOpacity(); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<class UMaterialInterface> VignetteMaterial;
	TWeakObjectPtr<APawn> HitPawn;
	double StartedAt = 0.0;
	float StartOpacity = 0.0f;
	float PeakOpacity = 0.0f;
};
