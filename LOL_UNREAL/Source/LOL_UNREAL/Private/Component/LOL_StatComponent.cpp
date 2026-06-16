	// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_StatComponent.h"
#include "Component/LOL_UIComponent.h"
#include "BaseChampion.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "LOL_HUD.h"

ULOL_StatComponent::ULOL_StatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UDataTable> ChampionStatTableObject(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionStats.Data_ChampionStats"));
	if (ChampionStatTableObject.Succeeded())
	{
		ChampionStatDataTable = ChampionStatTableObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> MinionStatTableObject(TEXT("/Game/LOL_Data/Data_Minions/Data_MinionStats.Data_MinionStats"));
	if (MinionStatTableObject.Succeeded())
	{
		MinionStatDataTable = MinionStatTableObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> JungleMonsterStatTableObject(TEXT("/Game/LOL_Data/Data_JungleMonsters/Data_JungleMonsterStats.Data_JungleMonsterStats"));
	if (JungleMonsterStatTableObject.Succeeded())
	{
		JungleMonsterStatDataTable = JungleMonsterStatTableObject.Object;
	}
}
void ULOL_StatComponent::BeginPlay()
{
	Super::BeginPlay();
}
void ULOL_StatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


inline void ULOL_StatComponent::SetHP(float NewHP)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		CurrentHP = FMath::Clamp<float>(NewHP, 0, BaseStat.MaxHP);

		OnRep_CurrentHP();
	}
}
inline void ULOL_StatComponent::SetMP(float NewMP)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		CurrentMP = FMath::Clamp<float>(NewMP, 0, BaseStat.MaxMP);

		OnRep_CurrentMP();
	}
}

inline void ULOL_StatComponent::SetStat(FChampionStat NewStat)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		BaseStat = NewStat;

		OnRep_BaseStat();
	}
}

void ULOL_StatComponent::InitializeStat()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UDataTable* TableToUse = nullptr;
	FName RowName = NAME_None;

	if (ABaseChampion* Champion = Cast<ABaseChampion>(Owner))
	{
		TableToUse = ChampionStatDataTable;
		RowName = Champion->GetChampionName();
	}
	else if (ABaseMinion* Minion = Cast<ABaseMinion>(Owner))
	{
		TableToUse = MinionStatDataTable;
		RowName = Minion->GetMinionName();
	}
	else if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(Owner))
	{
		TableToUse = JungleMonsterStatDataTable;
		RowName = JungleMonster->GetJungleMonsterName();
	}

	if (TableToUse && !RowName.IsNone())
	{
		FChampionStat* FoundRow = TableToUse->FindRow<FChampionStat>(RowName, TEXT(""));
		if (FoundRow)
		{
			SetStat(*FoundRow);
			SetHP(BaseStat.MaxHP);
			SetMP(BaseStat.MaxMP);
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
	if (OnStatChanged.IsBound())
	{
		OnStatChanged.Broadcast(BaseStat);
	}
}

void ULOL_StatComponent::OnRep_CurrentHP()
{
	if (OnHpChanged.IsBound()) OnHpChanged.Broadcast(CurrentHP);

}

void ULOL_StatComponent::OnRep_CurrentMP()
{
	if (OnMpChanged.IsBound()) OnMpChanged.Broadcast(CurrentMP);
}

float ULOL_StatComponent::ApplyDamage(float InDamage, EDamageType DamageType)
{
	if (InDamage <= 0.f || CurrentHP <= 0.f) return 0.f;

	float FinalDamage = CalculateReducedDamage(InDamage, DamageType);

	const float ActualDamage = FMath::Clamp(FinalDamage, 0.f, CurrentHP);
	SetHP(CurrentHP - ActualDamage);

	if (CurrentHP <= 0.f)
	{
		OnHpZero.Broadcast();
	}

	return ActualDamage;
}

float ULOL_StatComponent::CalculateReducedDamage(float RawDamage, EDamageType Type)
{
	float DefenseStat = 0.f;
	float PenPercent = 0.f;
	float PenFlat = 0.f;

	// 1. 데미지 타입에 따른 방어력 및 관통력 스탯 선택
	if (Type == EDamageType::Physical)
	{
		DefenseStat = BaseStat.Armor;
		PenPercent = BaseStat.PhysicalPenetrationPercent; 
		PenFlat = BaseStat.PhysicalPenetration;           
	}
	else if (Type == EDamageType::Magic)
	{
		DefenseStat = BaseStat.SpellBlock;
		PenPercent = BaseStat.MagicPenetrationPercent;    
		PenFlat = BaseStat.MagicPenetration;          
	}
	else if (Type == EDamageType::TrueDamage)
	{
		return RawDamage;
	}

	// 롤 공식: 최종 방어력 = (기본 방어력 * (1 - 퍼센트관통력)) - 고정관통력
	float EffectiveDefense = (DefenseStat * (1.0f - PenPercent)) - PenFlat;

	EffectiveDefense = FMath::Max(0.f, EffectiveDefense);

	// 3. 최종 데미지 배율 적용 100 / (100 + 방어력)
	float DamageMultiplier = 100.f / (100.f + EffectiveDefense);

	return RawDamage * DamageMultiplier;
}
