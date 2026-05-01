// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_StatComponent.generated.h"

// 스탯 구조체
USTRUCT(BlueprintType)
struct FChampionStat : public FTableRowBase
{
	GENERATED_BODY()

public:
	//------------------------------------------
	// =============== 가변 능력치 ===============
	UPROPERTY(EditAnywhere, Category = "Stat|Live")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, Category = "Stat|Live")
	float CurrentHP{};

	UPROPERTY(VisibleAnywhere, Category = "Stat|Live")
	float CurrentMP{};

	//------------------------------------------
	// =========== 기본 능력치 (Base) ===========

	// --- 체력(HP) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float MaxHP{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float HPPerLevel{};

	// --- 마나(MP) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float MaxMP{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float MPPerLevel{};

	// --- 방어력 / 마법 저항력 ---
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float Armor{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float ArmorPerLevel{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float SpellBlock{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float SpellBlockPerLevel{};

	// --- 공격력 / 공격 속도 ---
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float AttackDamage{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float AttackDamagePerLevel{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float AttackSpeed{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float AttackSpeedPerLevel{};

	// --- 이동 속도 / 사거리 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Base")
	float MoveSpeed{};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat|Base")
	float AttackRange{};

	// --- 자원 재생 ---
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float HPRegen{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float HPRegenPerLevel{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float MPRegen{};
	UPROPERTY(EditAnywhere, Category = "Stat|Base")
	float MPRegenPerLevel{};

	//------------------------------------------
	// ==== 추가/특수 능력치 (Bonus/Advanced) ====
	
	// 아이템으로 얻을 수 있는 추가 능력치
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusMaxHP{};          // 루비 수정 등
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusMaxMP{};          // 사파이어 수정 등
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusAttackDamage{};   // 롱소드 등
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusAttackSpeed{};    // 단검 등 (%)
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusArmor{};          // 천 갑옷 등
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusSpellBlock{};     // 마법무효화 망토 등
	UPROPERTY(VisibleAnywhere, Category = "Stat|Bonus")
	float BonusMoveSpeed{};      // 장화 등

	// --- 주문력 (Ability Power) / 적응형 능력치 (AdaptiveForce) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float AbilityPower{}; // 주문력 (AP)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float AdaptiveForce{}; // 적응형 능력치 (AF)

	// --- 관통력 (Penetration) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float PhysicalPenetration{}; // 물리 관통력 (고정)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float PhysicalPenetrationPercent{}; // 방어구 관통력 (%)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float MagicPenetration{}; // 마법 관통력 (고정)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float MagicPenetrationPercent{}; // 마법 관통력 (%)

	// --- 흡혈 (Vampirism) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float LifeSteal{};       // 생명력 흡수 (기본 공격)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float PhysicalVamp{};    // 물리 피해 흡혈
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float SpellVamp{};       // 주문 흡혈
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float Omnivamp{};        // 모든 피해 흡혈

	// --- 유틸리티 및 강화 ---
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float AbilityHaste{}; // 스킬 가속 (쿨감)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float Tenacity{}; // 강인함 (%)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float HealShieldPower{}; // 회복 및 보호막 효과 (%)

	// --- 크리티컬 (Critical) ---
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float CriticalChance{}; // 치명타 확률 (0~100)
	UPROPERTY(EditAnywhere, Category = "Stat|Advanced")
	float CriticalDamage = 1.75f; // 치명타 피해량 (기본 1.75배)
};

DECLARE_MULTICAST_DELEGATE(FOnHpZeroDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHpChangedDelegate, float /*CurrentHp*/);

DECLARE_MULTICAST_DELEGATE(FOnMpZeroDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMpChangedDelegate, float /*CurrentMp*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_StatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_StatComponent();
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:	
	FOnHpZeroDelegate OnHpZero;
	FOnHpChangedDelegate OnHpChanged;

	FOnMpZeroDelegate OnMpZero;
	FOnMpChangedDelegate OnMpChanged;

	float ApplyDamage(float InDagame);

	void SetHp(float NewHp);
	void SetMp(float NewMp);

	FORCEINLINE const FChampionStat GetStat() const { return BaseStat; }

	void InitializeStat();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	// 능력치 관련
	UPROPERTY(ReplicatedUsing = OnRep_BaseStat, EditAnywhere, Category = "Stat|Data")
	FChampionStat BaseStat;

	UFUNCTION()
	void OnRep_BaseStat();

	UPROPERTY(EditAnywhere, Category = "Stat|Data")
	class UDataTable* ChampionDataTable;
};
