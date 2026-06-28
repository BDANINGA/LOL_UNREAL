#include "ChampionSelect/GS_ChampSelect.h"
#include "Net/UnrealNetwork.h"

AGS_ChampSelect::AGS_ChampSelect()
{
    bReplicates = true;
}

void AGS_ChampSelect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGS_ChampSelect, TimeLeft);
}

void AGS_ChampSelect::OnRep_TimeLeft()
{
    OnTimerChanged.Broadcast(TimeLeft); // UI에게 시간이 바뀌었음을 알림
}