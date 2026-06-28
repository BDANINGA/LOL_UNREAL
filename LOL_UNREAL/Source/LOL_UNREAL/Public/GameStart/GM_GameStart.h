#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GM_GameStart.generated.h"

UCLASS()
class LOL_UNREAL_API AGM_GameStart : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGM_GameStart();

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
