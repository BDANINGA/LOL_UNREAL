// 챔피언의 기본 설정
// ----------------------------------------------------------------------------------
#pragma once
#include "GamePlayTag/LOL_GamePlayTags.h"
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
	TArray<UAnimMontage*> AttackMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> QMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> WMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> EMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> RMontage;
	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> PMontage;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TArray<UStaticMesh*> ProjectileMesh;
};

UCLASS()
class LOL_UNREAL_API ABaseChampion : public ACharacter, public IGameplayTagAssetInterface
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

	// 데이터테이블 연결 관련
	UDataTable* DataTable;

	UPROPERTY()
	FChampionResourceData ChampionResource;

	FName GetChampionName() const { return ChampionName; }
	void SetChampionData(FName RowName);
	
	// 스킬 관련
	void PressSkill(const uint8 skilltype);
	virtual void Skill_Q() {};
	virtual void Skill_W() {};
	virtual void Skill_E() {};
	virtual void Skill_R() {};

	// 태그 관련
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
	FGameplayTagContainer StatusTags;

	void AddStatusTag(FGameplayTag Tag);
	void RemoveStatusTag(FGameplayTag Tag);
	bool HasStatusTag(FGameplayTag Tag) const;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	// 공격 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	class USphereComponent* AttackRangeSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<ABaseChampion*> EnemiesInRange;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CombatTarget;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* HitTarget = nullptr;

	UFUNCTION(Server, Reliable)
	void Server_ExecuteAttackHit();

	void ProcessMoveInput(FVector ClickLocation, AActor* TargetActor);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ProcessMoveInput(FVector ClickLocation, AActor* TargetActor, bool bIsSearch);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayMontage(UAnimMontage* AnimMontage, float InplayRate);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetTargetAndPlayMontage(UAnimMontage* AnimMontage, float InplayRate, FRotator TargetRotation);

	FORCENOINLINE bool GetIsPressA() const { return bIsPressA; }
	void SetIsPressA(bool toggle);

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
	
	void SetIsKnockedBack(bool bInKnockback);

	virtual void OnBasicAttackHit(ACharacter* Target) {};

	int32 GetAM_Atk_Idx() { return AM_Atk_Idx; }
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

	UPROPERTY(Replicated)
	int32 AM_Atk_Idx{};
	UPROPERTY(Replicated)
	int32 AM_SKIll_Q_IDX{};
	UPROPERTY(Replicated)
	int32 AM_SKIll_W_IDX{};
	UPROPERTY(Replicated)
	int32 AM_SKIll_E_IDX{};
	UPROPERTY(Replicated)
	int32 AM_SKIll_R_IDX{};
	UPROPERTY(Replicated)
	int32 AM_SKIll_P_IDX{};

	UFUNCTION()
	void OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEnemyLeaveRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
