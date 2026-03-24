// Fill out your copyright notice in the Description page of Project Settings.
// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// 1. 이동
// 2. 공격
// ---------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LOL_PlayerController.generated.h"

/**
 *
 */
UCLASS()
class LOL_UNREAL_API ALOL_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALOL_PlayerController();
	// 서버 이동 요청 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetTargetLocation(FVector NewLocation);
	// 서버 타겟 설정 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetCombatTarget(AActor* Target);

	void SetIsMoving(bool bMove) { bIsMoving = bMove; }

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	
	UPROPERTY(Replicated)
	FVector TargetLocation;
	UPROPERTY(Replicated)
	bool bIsMoving;

	virtual void SetupInputComponent() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ClickMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillQAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillWAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillEAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillRAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SpaceBarAction;

	void OnClickMove();
	void OnSkillQ();
	void OnSkillW();
	void OnSkillE();
	void OnSkillR();
	void OnSpaceBarPressed();
	void OnSpaceBarReleased();
};
