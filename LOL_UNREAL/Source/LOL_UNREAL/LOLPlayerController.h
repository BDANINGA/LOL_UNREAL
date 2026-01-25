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
	virtual void PlayerTick(float DeltaTime) override; // 매 프레임 실행되는 함수
	
};
