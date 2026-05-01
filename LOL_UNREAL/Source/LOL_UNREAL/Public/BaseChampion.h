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
	ABaseChampion();

	UPROPERTY()
	class UNiagaraSystem* ClickFX;

	// 스탯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_StatComponent* StatComponent;

	// 공격 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_AttackComponent* AttackComponent;

	// 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_MoveComponent* MoveComponent;

	// 사망, 리스폰 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_LifeCycleComponent* LifeCycleComponent;

	// HP, MP 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_UIComponent* UIComponent;

	FName GetChampionName(){ return ChampionName; }

	// 추가: 캐릭터 스킬 함수 구현
	virtual void Skill_Q();
	virtual void Skill_W();
	virtual void Skill_E();
	virtual void Skill_R();

	//스턴 변수
	void ApplyStun(float Duration);
	void ClearStun();
	void MoveForward(float Value);

	FTimerHandle StunHandle;

	bool bIsStunned = false;

	UPROPERTY(BlueprintReadWrite, Category = "Status")
	bool bIsKnockedBack = false;

	// 넉백 해제용 함수
	void FinishKnockback() { bIsKnockedBack = false; }

	void ProcessMoveInput(FVector ClickLocation, AActor* TargetActor);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ProcessMoveInput(FVector ClickLocation, AActor* TargetActor);

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;
	
	void SetIsKnockedBack(bool bInKnockback);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage(FRotator TargetRotation);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
	class UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FName ChampionName;
};
