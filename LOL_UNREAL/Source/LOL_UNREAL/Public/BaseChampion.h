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
#include "BaseChampion.generated.h"

UCLASS()
class LOL_UNREAL_API ABaseChampion : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseChampion();

	// 스탯 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_StatComponent* StatComponent;

	// 카메라 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_CameraControlComponent* CameraControlComponent;

	void SetCameraLock(bool bLock);

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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void StartAttack();
	bool bCanAttack = true;
	FTimerHandle AttackTimerHandle;
	void ResetAttack();
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathMontage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
	class UAnimMontage* DeathMontage;

	bool bIsDead = false;

	// 죽었을 때 실행될 함수
	void Server_HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	virtual void OnDeath();

	//UI Widget Section (HP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"));
	TObjectPtr<class UWidgetComponent> HpBar;

	// 추가: UI Widget Section (MP)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> MpBar;
};