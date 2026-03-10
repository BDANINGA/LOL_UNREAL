// Fill out your copyright notice in the Description page of Project Settings.
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "Champion_Alistar.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();
    DefaultPawnClass = AChampion_Alistar::StaticClass();
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();
}