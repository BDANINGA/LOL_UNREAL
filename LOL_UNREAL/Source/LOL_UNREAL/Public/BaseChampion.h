// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.h
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// 3. 공격 대상 지정
// ----------------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ChampionWidgetInterface.h"
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

UCLASS()
class LOL_UNREAL_API ABaseChampion : public ACharacter, public IChampionWidgetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseChampion();

	// 카메라 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera;

	// 능력치 관련
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Stat")
	FChampionStat BaseStat;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 공격 대상 지정
	void SetCombatTarget(AActor* Target);

	// 공격 대상
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;

	// 매 프레임 사거리를 체크
	void CheckAttackRange();

	// 추가: 캐릭터 스킬 함수 구현
	void Skill_Q();
	void Skill_W();
	void Skill_E();
	void Skill_R();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void StartAttack();
	bool bCanAttack = true;
	FTimerHandle AttackTimerHandle;
	void ResetAttack();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	//Stat Section
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Stat, Meta = (AllowPrivateAccess = "true"));
	TObjectPtr<class UChampionComponent> Stat;

	//UI Widget Section (HP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"));
	TObjectPtr<class UWidgetComponent> HpBar;

	virtual void SetupCharacterWidget(class UChampionUserWidget* InUserWidget) override;

	// 추가: UI Widget Section (MP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> MpBar;

};