#include "GameStart/GM_GameStart.h"

#include "GameStart/PC_GameStart.h"

AGM_GameStart::AGM_GameStart()
{
	PlayerControllerClass = APC_GameStart::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

void AGM_GameStart::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// This map is UI-only, so the local controller does not need a pawn.
}
