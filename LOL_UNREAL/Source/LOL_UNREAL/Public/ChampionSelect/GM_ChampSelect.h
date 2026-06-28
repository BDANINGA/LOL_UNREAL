#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GM_ChampSelect.generated.h"

UCLASS()
class LOL_UNREAL_API AGM_ChampSelect : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGM_ChampSelect();
    virtual void BeginPlay() override;

    // 모든 플레이어가 픽을 완료했는지 체크하는 함수
    void CheckAllPlayersReady();

    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

protected:
    FTimerHandle TimerHandle_MatchCountdown;
    void AdvanceTimer();
    void StartInGameMatch();
};