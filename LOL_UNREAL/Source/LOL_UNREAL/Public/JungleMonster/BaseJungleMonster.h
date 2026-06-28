// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "BaseJungleMonster.generated.h"

USTRUCT(BlueprintType)
struct FJungleMonsterResourceData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "UI")
	TObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, Category = "ABP")
	TSubclassOf<UAnimInstance> AnimBlueprint;

	UPROPERTY(EditAnywhere, Category = "AM")
	TArray<TObjectPtr<UAnimMontage>> AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TArray<TObjectPtr<UStaticMesh>> ProjectileMesh;
};

UCLASS()
class LOL_UNREAL_API ABaseJungleMonster : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseJungleMonster();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_StatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_AttackComponent> AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_MoveComponent> MoveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_LifeCycleComponent> LifeCycleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_UIComponent> UIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|LOL")
	TObjectPtr<class ULOL_StateComponent> StateComponent;

	UPROPERTY()
	FJungleMonsterResourceData JungleMonsterResource;

	virtual FName GetJungleMonsterName() const { return JungleMonsterName; }
	FVector GetSpawnLocation() const { return SpawnLocation; }
	FRotator GetSpawnRotation() const { return SpawnRotation; }
	bool IsStationaryMonster() const { return bStationaryMonster; }
	void InitializeJungleMonster(FName RowName);
	void SetJungleMonsterData(FName RowName);
	void ApplyCrowdControl(float Duration);

protected:
	FVector GetMeshScaleForMonster(FName RowName) const;
	void StartReturnToSpawn();
	void UpdateReturnToSpawn(float DeltaTime);
	bool ShouldResetLeash() const;
	void ClearCrowdControl();

	UFUNCTION()
	void OnRep_JungleMonsterName();

	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_JungleMonsterName, Category = "JungleMonster|Data")
	FName JungleMonsterName;

	UPROPERTY(EditDefaultsOnly, Category = "JungleMonster|Data")
	TObjectPtr<UDataTable> DataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JungleMonster|Leash")
	float LeashRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JungleMonster|Leash")
	float ReturnAcceptanceRadius = 50.0f;

	UPROPERTY()
	FVector SpawnLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator SpawnRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bStationaryMonster = false;

	bool bReturningToSpawn = false;
	bool bCrowdControlled = false;

	FTimerHandle CrowdControlTimerHandle;

};
