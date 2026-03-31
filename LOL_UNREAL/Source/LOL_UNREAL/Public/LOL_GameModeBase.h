// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaseChampion.h"

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

    // 플레이어 컨트롤러에 따른 캐릭터 클래스 반환 함수
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    void RequestRespawn(ABaseChampion* DeadChampion);

    // 게임 시작 시 호출되는 함수
    virtual void BeginPlay() override;
protected:
    UPROPERTY()
    TSubclassOf<APawn> AlistarClass;

    UPROPERTY()
    TSubclassOf<APawn> VayneClass;
};
