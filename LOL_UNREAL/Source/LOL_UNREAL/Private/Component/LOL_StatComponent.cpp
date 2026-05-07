	// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_StatComponent.h"
#include "BaseChampion.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "LOL_HUD.h"

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

			ChampionStatUpdate();
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

float ULOL_StatComponent::ApplyDamage(float InDamage, EDamageType DamageType)
{
	if (InDamage <= 0.f || BaseStat.CurrentHP <= 0.f) return 0.f;

	float FinalDamage = CalculateReducedDamage(InDamage, DamageType);

	const float ActualDamage = FMath::Clamp(FinalDamage, 0.f, BaseStat.CurrentHP);
	SetHp(BaseStat.CurrentHP - ActualDamage);

	if (BaseStat.CurrentHP <= 0.f)
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

void ULOL_StatComponent::ChampionStatUpdate()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		if (OwnerPawn)
		{
			APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
			if (PC && PC->GetHUD())
			{
				ALOL_HUD* MyHUD = Cast<ALOL_HUD>(PC->GetHUD());
				if (MyHUD)
				{
					MyHUD->UpdateAttackDamage(BaseStat.AttackDamage);
					MyHUD->UpdateAbilityPower(BaseStat.AbilityPower);
					MyHUD->UpdateArmor(BaseStat.Armor);
					MyHUD->UpdateSpellBlock(BaseStat.SpellBlock);
					MyHUD->UpdateAttackSpeed(BaseStat.AttackSpeed);
					MyHUD->UpdateAbilityHaste(BaseStat.AbilityHaste);
					MyHUD->UpdateCriticalRate(BaseStat.CriticalChance);
					MyHUD->UpdateMoveSpeed(BaseStat.MoveSpeed);
				}
			}
		}
	}
}