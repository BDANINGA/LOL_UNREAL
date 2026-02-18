// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LOL_GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class LOL_UNREAL_API ALOL_GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
    ALOL_GameModeBase();

    // 게임 시작 시 호출되는 함수
    virtual void BeginPlay() override;
};
