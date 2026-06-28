#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PC_Lobby.generated.h"

UCLASS()
class LOL_UNREAL_API APC_Lobby : public APlayerController
{
	GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> LobbyWidgetClass;

    UPROPERTY()
    UUserWidget* LobbyWidget;

    // 서버에 내 닉네임 등록 요청
    UFUNCTION(Server, Reliable)
    void Server_SetNickname(const FString& NewNickname);

    // 서버에 팀 변경 요청 (1: Left, 2: Right)
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Lobby")
    void Server_RequestChangeTeam(uint8 TargetTeamID);

    // 호스트 전용 게임 시작 요청
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Lobby")
    void Server_StartGame();
};
