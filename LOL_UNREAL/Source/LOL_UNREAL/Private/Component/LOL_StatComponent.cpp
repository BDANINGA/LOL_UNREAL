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
	BaseStat.AttackDamage = 25.0f;

}
void ULOL_StatComponent::BeginPlay()
{
	Super::BeginPlay();

	SetHp(BaseStat.MaxHP);
	
}
void ULOL_StatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


inline void ULOL_StatComponent::SetHp(float NewHp)
{
	BaseStat.CurrentHP = FMath::Clamp<float>(NewHp, 0, BaseStat.MaxHP);

	if (OnHpChanged.IsBound())
	{
		OnHpChanged.Broadcast(BaseStat.CurrentHP);
	}
}

void ULOL_StatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 컴포넌트가 소유한 BaseStat을 복제 등록합니다.
	DOREPLIFETIME(ULOL_StatComponent, BaseStat);
}

void ULOL_StatComponent::OnRep_BaseStat()
{
	// 클라이언트의 위젯들에게 HP가 변했음을 알립니다.
	if (OnHpChanged.IsBound())
	{
		OnHpChanged.Broadcast(BaseStat.CurrentHP);
	}
}

float ULOL_StatComponent::ApplyDamage(float InDamage)
{
	if (InDamage <= 0.f || BaseStat.CurrentHP <= 0.f) return 0.f;

	const float ActualDamage = FMath::Clamp(InDamage, 0.f, BaseStat.CurrentHP);
	SetHp(BaseStat.CurrentHP - ActualDamage);

	if (BaseStat.CurrentHP <= 0.f)
	{
		OnHpZero.Broadcast(); // 사망 이벤트 발생
	}

	return ActualDamage;
}

