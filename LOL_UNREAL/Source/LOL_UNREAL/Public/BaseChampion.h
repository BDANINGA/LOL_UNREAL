// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.h
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// ----------------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseChampion.generated.h"

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

	// --- 공격력(Attack) ---
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

UCLASS()
class LOL_UNREAL_API ABaseChampion : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseChampion();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 카메라 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera;

	// 능력치 관련
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Stat")
	FChampionStat BaseStat;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};