// 미니언 기본 뼈대
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseMinion.generated.h"

// 미니언 리소스 데이터 테이블
USTRUCT(BlueprintType)
struct FMinionResourceData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	USkeletalMesh* Mesh;

	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* Portrait;

	UPROPERTY(EditAnywhere, Category = "ABP")
	TSubclassOf<UAnimInstance> AnimBlueprint;

	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<UAnimMontage*> AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TArray<UStaticMesh*> ProjectileMesh;
};

UCLASS()
class LOL_UNREAL_API ABaseMinion : public APawn
{
	GENERATED_BODY()

public:
	ABaseMinion();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Pawn")
	class UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Pawn")
	class USkeletalMeshComponent* MeshComponent;

	// ------------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_StatComponent* StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_AttackComponent* AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_MoveComponent* MoveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_LifeCycleComponent* LifeCycleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_UIComponent* UIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	class ULOL_StateComponent* StateComponent;

	// -------------------------------------------------------------------------------
	FORCEINLINE class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

	UDataTable* DataTable;

	UPROPERTY()
	FMinionResourceData MinionResource;

	virtual FName GetMinionName() const { return MinionName; };
	void SetMinionData(FName RowName);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Minion|Data")
	FName MinionName;
};
