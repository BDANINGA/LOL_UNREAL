// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_StatComponent.generated.h"

// 스텟 구조체
USTRUCT(BlueprintType)
struct FChampionStat
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	int32 Level = 1;

	// --- 체력(HP) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HPPerLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HPRegen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float HPRegenPerLevel;

	// --- 마나(MP) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxMP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentMP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MPPerLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MPRegen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MPRegenPerLevel;

	// --- 속도(MoveSpeed) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MoveSpeed;

	// --- 방어력(Armor) / 마법 저항력(SpellBlock) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Armor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float ArmorPerLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float SpellBlock;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float SpellBlockPerLevel;

	// --- 공격력(AttackDamege) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackDamagePerLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackSpeedPerLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackRange;

	// --- 크리티컬(Critical) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Critical;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CriticalPerLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float ADPerLevel;
};

DECLARE_MULTICAST_DELEGATE(FOnHpZeroDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHpChangedDelegate, float /*CurrentHp*/);

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

	float ApplyDamage(float InDagame);
	void SetHp(float NewHp);
	FORCEINLINE const FChampionStat GetStat() const { return BaseStat; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	// 능력치 관련
	UPROPERTY(ReplicatedUsing = OnRep_BaseStat, EditAnywhere, Category = "Stat")
	FChampionStat BaseStat;

	UFUNCTION()
	void OnRep_BaseStat();
};
