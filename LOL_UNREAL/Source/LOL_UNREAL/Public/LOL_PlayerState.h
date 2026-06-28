#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/LOL_GameInstance.h"
#include "LOL_PLayerState.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_PLayerState : public APlayerState
{
	GENERATED_BODY()

public:
    // 다른 클라이언트에서도 보이도록 Replicated
    UPROPERTY(Replicated, BlueprintReadWrite)
    FString Nickname;

    UPROPERTY(Replicated, BlueprintReadWrite)
    uint8 TeamID;

    UPROPERTY(Replicated, BlueprintReadWrite)
    EChampionID SelectedChampion;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};