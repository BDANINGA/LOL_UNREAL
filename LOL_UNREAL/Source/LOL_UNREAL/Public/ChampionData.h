#pragma once

#include "CoreMinimal.h"
#include "ChampionData.generated.h"

USTRUCT(BlueprintType)
struct FChampionBaseStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Health = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HealthGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HealthRegen = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HealthRegenGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Mana = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ManaGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ManaRegen = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ManaRegenGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackDamageGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackSpeedGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Armor = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ArmorGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MagicResistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MagicResistanceGrowth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AttackRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed = 0.f;
};

USTRUCT(BlueprintType)
struct FChampionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChampionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FChampionBaseStats BaseStats;
};