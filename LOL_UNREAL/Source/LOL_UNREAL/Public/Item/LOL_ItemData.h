#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LOL_ItemData.generated.h"

USTRUCT(BlueprintType)
struct LOL_UNREAL_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 SellPrice = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusMaxHP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusMaxMP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusAttackDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusAbilityPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusAttackSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusArmor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusSpellBlock = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusMoveSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusCriticalChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusLifeSteal = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusAbilityHaste = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusHealShieldPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusPhysicalPenetrationPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Bonus")
	float BonusMagicPenetrationPercent = 0.0f;
};
