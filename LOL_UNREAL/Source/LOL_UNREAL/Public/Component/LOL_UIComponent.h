// HP. MP 위젯 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_UIComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedDelegate, float /*CurrentValue*/);
DECLARE_MULTICAST_DELEGATE(FOnDeathDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_UIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_UIComponent();

	void UpdateHpFromStat(float NewHp);
	void UpdateMpFromStat(float NewMp);

	void UpdateLevel(const struct FChampionStat& CurrentStat);

	void SetMaxHp(float InMaxHp) { CachedMaxHP = InMaxHp; }
	void SetMaxMp(float InMaxMp) { CachedMaxMP = InMaxMp; }

	class UWidgetComponent* GetActorWidget() { return ActorWidget; }

	void ShowRangeIndicator();
	void HideRangeIndicator();

	void UpdateHPBarImage(UTexture2D* TargetTexture);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	class APawn* OwnerPawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* ActorWidget;

	UPROPERTY()
	class UDecalComponent* RangeIndicator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	class UMaterialInterface* BaseDecalMaterial;

	UPROPERTY()
	class UMaterialInstanceDynamic* DecalMID;

private:
	TSubclassOf<class UUserWidget> ChampionWidgetClass;
	TSubclassOf<class UUserWidget> MinionWidgetClass;

	float CachedMaxHP = -1.f;
	float CachedMaxMP = -1.f;
};
