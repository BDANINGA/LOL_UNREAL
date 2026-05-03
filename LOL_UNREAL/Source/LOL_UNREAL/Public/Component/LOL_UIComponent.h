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

	void SetMaxHp(float InMaxHp) { CachedMaxHP = InMaxHp; }
	void SetMaxMp(float InMaxMp) { CachedMaxMP = InMaxMp; }

	class UWidgetComponent* GetChampionWidget() { return ChampionWidget;}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* ChampionWidget;

private:
	float CachedMaxHP = -1.f;
	float CachedMaxMP = -1.f;
};
