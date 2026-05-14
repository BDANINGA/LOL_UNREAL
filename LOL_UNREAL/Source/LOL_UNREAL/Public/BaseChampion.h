// 챔피언의 기본 설정
// ----------------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseChampion.generated.h"

// 챔피언 리소스 데이터 테이블
USTRUCT(BlueprintType)
struct FChampionData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Name")
	FString ChampionName;

	UPROPERTY(EditAnywhere, Category = "Mesh")
	USkeletalMesh* Mesh;

	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait_Circle;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait_Loading;

	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillP_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillQ_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillW_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillE_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillR_Image;

	UPROPERTY(EditAnywhere, Category = "ABP")
	TSubclassOf<UAnimInstance> AnimBlueprint;

	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* AttackMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* QMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* WMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* EMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* RMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	UAnimMontage* PMontage;
};

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

	// 스킬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UChampion_SkillComponent* SkillComponent;

	// UI Image
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait_Circle;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait_Loading;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillQ_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillW_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillE_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillR_Image;
	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* SkillP_Image;

	FName GetChampionName() const { return ChampionName; }

	void PressSkill(char skilltype);

	virtual void SetChampionData(FName RowName) {};
	virtual void Skill_Q() {};
	virtual void Skill_W() {};
	virtual void Skill_E() {};
	virtual void Skill_R() {};

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	class USphereComponent* AttackRangeSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<ABaseChampion*> EnemiesInRange;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage(FRotator TargetRotation);
	
	void SetIsKnockedBack(bool bInKnockback);
	
	FORCENOINLINE bool GetIsPressA() const { return bIsPressA; }
	void SetIsPressA(bool toggle);

	virtual void OnBasicAttackHit(ACharacter* Target) {};

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FName ChampionName;
	
	UPROPERTY()
	bool bIsPressA = false;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* PMontage;

	UFUNCTION()
	void OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEnemyLeaveRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
