// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LOLPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class LOL_UNREAL_API ALOLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALOLPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	// 서버 이동 요청 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetTargetLocation(FVector NewLocation);

	FVector TargetLocation;
	bool bIsMoving;
	
};
