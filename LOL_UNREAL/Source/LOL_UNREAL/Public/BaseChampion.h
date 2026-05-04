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

	// 스탯 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULOL_StatComponent* StatComponent;

	void SetCameraLock(bool bLock);

	// 매 프레임 사거리를 체크
	void CheckAttackRange();

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

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnRespawn();

	void Respawn();


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	bool bIsDead = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	FVector TargetLocation;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	void ProcessMoveInput(FVector ClickLocation, AActor* TargetActor);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ProcessMoveInput(FVector ClickLocation, AActor* TargetActor);

	// 공격 대상
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;

	void SetAttackTarget(AActor* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAttackTarget(AActor* Target);
	
	void SetIsKnockedBack(bool bInKnockback);

	// 2026 05 01 
	virtual void OnBasicAttackHit(ACharacter* Target) {}

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
	void Multicast_PlayAttackMontage(FRotator TargetRotation);

	//2026 05 01 (q,w,e,r모션 만들기 위해 더 추가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* QMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* WMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* EMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* RMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
	class UAnimMontage* DeathMontage;

	void Server_HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	virtual void OnDeath();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"));
	TObjectPtr<class UWidgetComponent> HpBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWidgetComponent> MpBar;

	UPROPERTY(BlueprintReadWrite, Category = "Status")
	bool bIsKnockedBack = false;

	// 넉백 해제용 함수
	void FinishKnockback() { bIsKnockedBack = false; }
};
