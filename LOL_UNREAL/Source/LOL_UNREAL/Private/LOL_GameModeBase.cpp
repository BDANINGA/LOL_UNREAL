// Fill out your copyright notice in the Description page of Project Settings.
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_Character.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();

    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/vain/bp_lolcharacter"));
    if (PlayerPawnBPClass.Class != NULL)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();
}