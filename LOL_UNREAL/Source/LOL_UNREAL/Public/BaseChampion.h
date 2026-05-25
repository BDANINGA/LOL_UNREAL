// 챔피언의 기본 설정
// ----------------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseChampion.generated.h"

// 챔피언 리소스 데이터 테이블
USTRUCT(BlueprintType)
struct FChampionResourceData : public FTableRowBase
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

	UDataTable* DataTable;

	UPROPERTY()
	FChampionResourceData ChampionResource;

	FName GetChampionName() const { return ChampionName; }

	void SetChampionData(FName RowName);

	void PressSkill(const uint8 skilltype);

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
	void Server_ProcessMoveInput(FVector ClickLocation, AActor* TargetActor, bool bIsSearch);

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

	//베인 벽꿍 관련 함수
	void StartKnockbackWithWallCheck(const FVector& InLaunchVelocity, float MaxKnockbackTime, float InWallStunDuration);
	void CheckKnockbackWall();
	void EndKnockback();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ApplyStun(float Duration);

	FVector KnockbackDirection = FVector::ZeroVector;
	float   PendingWallStunDuration = 0.f;
	FTimerHandle KnockbackCheckHandle;
	FTimerHandle KnockbackTimeoutHandle;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool CanCastWhileStunned(uint8 skilltype) const { return false; }

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FName ChampionName;
	
	UPROPERTY()
	bool bIsPressA = false;

	UFUNCTION()
	void OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEnemyLeaveRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
