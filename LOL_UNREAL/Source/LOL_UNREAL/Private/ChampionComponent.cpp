// Fill out your copyright notice in the Description page of Project Settings.


#include "ChampionComponent.h"

// Sets default values for this component's properties
UChampionComponent::UChampionComponent()
{
	MaxHp = 500.0f;
	CurrentHp = MaxHp;
}


// Called when the game starts
void UChampionComponent::BeginPlay()
{
	Super::BeginPlay();

	SetHp(MaxHp);

}

float UChampionComponent::ApplyDamage(float InDamage)
{
	const float PreHp = CurrentHp;

	const float ActualDamage = FMath::Clamp<float>(InDamage, 0, InDamage);

	SetHp(PreHp - ActualDamage);
	if (CurrentHp <= KINDA_SMALL_NUMBER)
	{
		OnHpZero.Broadcast();
	}
	return ActualDamage;
}

inline void UChampionComponent::SetHp(float NewHp)
{
	CurrentHp = FMath::Clamp<float>(NewHp, 0, MaxHp);

	OnHpChanged.Broadcast(CurrentHp);
}


