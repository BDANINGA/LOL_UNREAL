#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LOL_Minion.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_Minion : public APawn
{
	GENERATED_BODY()

public:
	ALOL_Minion();

protected:
	virtual void BeginPlay() override;

	// ----------------------------- 컴포넌트 ------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UFloatingPawnMovement* MovementComp;

	//--------------------------------- 데이터 변수 -------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackRange = 150.f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MoveSpeed = 300.f;

	// 팀 구분 (Blue/Red)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	uint8 TeamID;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
