// Fill out your copyright notice in the Description page of Project Settings.

#include "LOL_ChampionHpBarWidget.h"
#include "Components/ProgressBar.h"

ULOL_ChampionHpBarWidget::ULOL_ChampionHpBarWidget(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	MaxHp = -1.0f;
}

void ULOL_ChampionHpBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HpProgressBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("HpBar")));
	ensure(HpProgressBar);
}

void ULOL_ChampionHpBarWidget::UpdateHpBar(float NewCurrentHp)
{
	ensure(MaxHp > 0.0f);
	if (HpProgressBar)
	{
		HpProgressBar->SetPercent(NewCurrentHp / MaxHp);
	}
}