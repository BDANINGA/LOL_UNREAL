#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PS_Lobby.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayerDataReplicated);

UCLASS()
class LOL_UNREAL_API APS_Lobby : public APlayerState
{
	GENERATED_BODY()

public:
    APS_Lobby();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_Nickname, BlueprintReadOnly, Category = "Lobby")
    FString Nickname;

    UPROPERTY(ReplicatedUsing = OnRep_TeamID, BlueprintReadOnly, Category = "Lobby")
    uint8 TeamID;

    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnLobbyPlayerDataReplicated OnPlayerDataReplicated;

protected:
    UFUNCTION()
    void OnRep_Nickname();

    UFUNCTION()
    void OnRep_TeamID();

    virtual void CopyProperties(APlayerState* NewPlayerState) override;
};
