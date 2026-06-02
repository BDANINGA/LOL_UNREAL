// Fill out your copyright notice in the Description page of Project Settings.
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"

#include "Component/LOL_LifeCycleComponent.h"

#include "Champion/Champion_Alistar.h"
#include "Champion/Champion_Vayne.h"
#include "Champion/Champion_Blitz.h"
#include "Champion/Champion_Garen.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();
    
    // 플레이어마다 캐릭터를 다르게 설정하기 위해서는 필요하지 않음.
    // DefaultPawnClass = AChampion_Alistar::StaticClass();

    HUDClass = ALOL_HUD::StaticClass();
}  

UClass* ALOL_GameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    // 1. 해당 컨트롤러가 '로컬'에서 실행되는 서버 컨트롤러인지 확인
    // 리슨 서버 모드에서 0번 플레이어(방장)는 IsLocalController()가 true이며, 서버 권한을 가집니다.
    if (InController && InController->IsLocalController())
    {
        // 첫 번째 플레이어(방장)는 알리스타
        return AChampion_Alistar::StaticClass();
    }

    // 2. 그 외에 접속하는 클라이언트 플레이어들
    return AChampion_Garen::StaticClass();
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();
}

void ALOL_GameModeBase::RequestRespawn(ABaseChampion* DeadChampion)
{
    float RespawnDelay = 5.0f; // 나중에는 레벨에 따라 계산식 적용

    ULOL_LifeCycleComponent* LifeCycleComp = DeadChampion->FindComponentByClass<ULOL_LifeCycleComponent>();

    if (LifeCycleComp)
    {
        FTimerHandle RespawnTimer;
        FTimerDelegate RespawnDelegate;

        // 핵심 수정: 바인딩 대상을 컴포넌트와 컴포넌트의 Respawn 함수로 변경합니다.
        RespawnDelegate.BindUObject(LifeCycleComp, &ULOL_LifeCycleComponent::Respawn);

        GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
    }
}