// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_StatComponent.h"
#include "BaseChampion.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"

ULOL_StatComponent::ULOL_StatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UDataTable> StatTableObject(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionStats.Data_ChampionStats"));
	if (StatTableObject.Succeeded())
	{
		ChampionDataTable = StatTableObject.Object;
	}
}
void ULOL_StatComponent::BeginPlay()
{
	Super::BeginPlay();

	SetHp(BaseStat.MaxHP);
	SetMp(BaseStat.MaxMP);
	
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
inline void ULOL_StatComponent::SetMp(float NewMp)
{
	BaseStat.CurrentMP = FMath::Clamp<float>(NewMp, 0, BaseStat.MaxMP);

	if (OnMpChanged.IsBound())
	{
		OnMpChanged.Broadcast(BaseStat.CurrentMP);
	}
}

void ULOL_StatComponent::InitializeStat()
{
	ABaseChampion* Owner = Cast<ABaseChampion>(GetOwner());
	if (Owner && ChampionDataTable)
	{
		FChampionStat* FoundRow = ChampionDataTable->FindRow<FChampionStat>(Owner->GetChampionName(), TEXT(""));

		if (FoundRow)
		{
			BaseStat = *FoundRow;
			BaseStat.CurrentHP = BaseStat.MaxHP;
			BaseStat.CurrentMP = BaseStat.MaxMP;
		}
	}
}

void ULOL_StatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULOL_StatComponent, BaseStat);
}

void ULOL_StatComponent::OnRep_BaseStat()
{
	// 클라이언트의 위젯들에게 스탯이 변했음을 알립니다.
	if (OnHpChanged.IsBound())
	{
		OnHpChanged.Broadcast(BaseStat.CurrentHP);
	}
	if (OnMpChanged.IsBound())
	{
		OnMpChanged.Broadcast(BaseStat.CurrentMP);
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
