#include "Component/LOL_StatComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StateComponent.h"

#include "BaseChampion.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Building/BaseBuilding.h"

#include "Minion/BaseMinion.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"

#include "LOL_HUD.h"
#include "LOL_GameState.h"

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

	static ConstructorHelpers::FObjectFinder<UDataTable> BuildingStatTableObject(TEXT("/Game/LOL_Data/Data_Buildings/Data_BuildingStats.Data_BuildingStats"));
	if (BuildingStatTableObject.Succeeded())
	{
		BuildingStatDataTable = BuildingStatTableObject.Object;
	}
}
void ULOL_StatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ALOL_GameState* GS = Cast<ALOL_GameState>(GetWorld()->GetGameState()))
	{
		GS->OnOneSecondEvent.AddUObject(this, &ULOL_StatComponent::HandleRegeneration);
	}
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
	else if (ABaseBuilding* Building = Cast<ABaseBuilding>(Owner))
	{
		TableToUse = BuildingStatDataTable;
		RowName = Building->GetBuildingName();
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
	DOREPLIFETIME(ULOL_StatComponent, CurrentHP);
	DOREPLIFETIME(ULOL_StatComponent, CurrentMP);
}

void ULOL_StatComponent::AddGold(float Amount)
{
	if (Amount <= 0.f) return;

	CurrentGold += Amount;

	OnRep_CurrentGold();
}

void ULOL_StatComponent::AddEXP(float Amount)
{
	if (Amount <= 0.f) return;

	if (BaseStat.Level >= 18) return;

	CurrentEXP += Amount;

	while (CurrentEXP >= MaxEXP)
	{
		CurrentEXP -= MaxEXP;
		BaseStat.Level++;

		MaxEXP = 280.f + (BaseStat.Level - 1) * 100.f;

		BaseStat.MaxHP += BaseStat.HPPerLevel;
		BaseStat.HPRegen += BaseStat.HPRegenPerLevel;
		BaseStat.MaxMP += BaseStat.MPPerLevel;
		BaseStat.MPRegen += BaseStat.MPRegenPerLevel;
		BaseStat.Armor += BaseStat.ArmorPerLevel;
		BaseStat.SpellBlock += BaseStat.SpellBlockPerLevel;
		BaseStat.AttackDamage += BaseStat.AttackDamagePerLevel;
		BaseStat.AttackSpeed += BaseStat.AttackSpeedPerLevel;
		

		SetHP(CurrentHP + BaseStat.HPPerLevel);
		SetMP(CurrentMP + BaseStat.MPPerLevel);

		// 클라이언트들에게 능력치가 변했음을 전파
		OnRep_BaseStat();
		OnRep_CurrentEXP();

		// 만렙 도달 시 탈출
		if (BaseStat.Level >= 18)
		{
			CurrentEXP = 0.f;
			break;
		}
	}
}

void ULOL_StatComponent::HandleRegeneration()
{
	ABaseChampion* OwnerChampion = Cast<ABaseChampion>(GetOwner());
	if (!OwnerChampion) return;

	ULOL_StateComponent* StateComp = OwnerChampion->FindComponentByClass<ULOL_StateComponent>();
	if(StateComp->HasStatusTag(LOLTags::State_Dead)) return;

	float HPRatio = BaseStat.HPRegen / 5.0f;
	float MPRatio = BaseStat.MPRegen / 5.0f;

	if (CurrentHP > 0.0f)
	{
		SetHP(CurrentHP + HPRatio);
	}

	if (CurrentMP > 0.0f)
	{
		SetMP(CurrentMP + MPRatio);
	}

	CurrentGold += 1;
	OnRep_CurrentGold();
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

void ULOL_StatComponent::OnRep_CurrentGold()
{
	if (OnGoldChanged.IsBound()) OnGoldChanged.Broadcast(CurrentGold);
}

void ULOL_StatComponent::OnRep_CurrentEXP()
{
	if (OnEXPChanged.IsBound()) OnEXPChanged.Broadcast(CurrentEXP);
}

float ULOL_StatComponent::ApplyDamage(float InDamage, EDamageType DamageType, AController* Instigator, AActor* Causer)
{
	if (InDamage <= 0.f || CurrentHP <= 0.f) return 0.f;

	float FinalDamage = CalculateReducedDamage(InDamage, DamageType);

	const float ActualDamage = FMath::Clamp(FinalDamage, 0.f, CurrentHP);
	SetHP(CurrentHP - ActualDamage);

	if (CurrentHP <= 0.f)
	{
		OnHpZero.Broadcast(Instigator, Causer);
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
