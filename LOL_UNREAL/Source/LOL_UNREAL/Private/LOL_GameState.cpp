#include "LOL_GameState.h"
#include "Net/UnrealNetwork.h"

void ALOL_GameState::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimer(OneSecondTimerHandle, this, &ALOL_GameState::TickOneSecond, 1.0f, true);
}

void ALOL_GameState::TickOneSecond()
{
    OnOneSecondEvent.Broadcast();
    CurrentMatchTime += 1;
}

void ALOL_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALOL_GameState, CurrentMatchTime);
}