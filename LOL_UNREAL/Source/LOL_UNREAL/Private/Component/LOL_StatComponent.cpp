// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_StatComponent.h"
#include "Net/UnrealNetwork.h"

ULOL_StatComponent::ULOL_StatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	BaseStat.MaxHP = 500.0f;
	BaseStat.CurrentHP = BaseStat.MaxHP;
	BaseStat.AttackRange = 500.0f;
	BaseStat.AttackSpeed = 1.0f;

}
void ULOL_StatComponent::BeginPlay()
{
	Super::BeginPlay();

	SetHp(BaseStat.MaxHP);
	
}

void ULOL_StatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 컴포넌트가 소유한 BaseStat을 복제 등록합니다.
	DOREPLIFETIME(ULOL_StatComponent, BaseStat);
}

float ULOL_StatComponent::ApplyDamage(float InDamage)
{
	const float PreHp = BaseStat.CurrentHP;

	const float ActualDamage = FMath::Clamp<float>(InDamage, 0, InDamage);

	SetHp(PreHp - ActualDamage);
	if (BaseStat.CurrentHP <= KINDA_SMALL_NUMBER)
	{
		OnHpZero.Broadcast();
	}
	return ActualDamage;
}

inline void ULOL_StatComponent::SetHp(float NewHp)
{
	BaseStat.CurrentHP = FMath::Clamp<float>(NewHp, 0, BaseStat.MaxHP);

	OnHpChanged.Broadcast(BaseStat.CurrentHP);
}

void ULOL_StatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}