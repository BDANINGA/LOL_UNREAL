#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GM_Lobby.generated.h"

UCLASS()
class LOL_UNREAL_API AGM_Lobby : public AGameModeBase
{
	GENERATED_BODY()

public:
    AGM_Lobby();

    virtual void OnPostLogin(AController* NewPlayer) override;

    // 팀 변경 로직
    void MovePlayerToTeam(APlayerController* Player, uint8 TargetTeamID);

    // 게임 시작 (픽창으로 이동)
    void StartMobaMatch();

    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

protected:
    // 팀별 인원 관리를 위한 배열
    UPROPERTY()
    TArray<APlayerController*> LeftTeam;
    UPROPERTY()
    TArray<APlayerController*> RightTeam;

private:
    bool bLobbyLocked = false;
};
