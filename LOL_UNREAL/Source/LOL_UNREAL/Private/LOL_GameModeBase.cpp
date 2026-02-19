// Fill out your copyright notice in the Description page of Project Settings.

#include "LOL_GameModeBase.h"
#include "LOL_GameModeBase.h"
#include "LOL_Character.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    // 1. 기본적으로 스폰할 캐릭터(Pawn) 설정
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT(""));
    if (PlayerPawnBPClass.Class != NULL)
    {
        DefaultPawnClass = 
    }

    // 2. 사용할 플레이어 컨트롤러 설정
    PlayerControllerClass = ALOL_PlayerController::StaticClass();

    // 3. 기타 클래스 설정 (HUD, GameState 등)
    // HUDClass = AMyHUD::StaticClass();
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();
    // 게임 시작 시 실행할 커스텀 로직 (예: 타이머 시작, 점수 초기화 등)
}
