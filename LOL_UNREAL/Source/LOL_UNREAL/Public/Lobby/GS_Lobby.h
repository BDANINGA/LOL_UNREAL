#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GS_Lobby.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamListChanged);

UCLASS()
class LOL_UNREAL_API AGS_Lobby : public AGameStateBase
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_TeamList)
    int32 TeamListVersion = 0;

    // 클라이언트가 변수 변경을 감지할 함수
    UFUNCTION()
    void OnRep_TeamList();

    UPROPERTY(BlueprintAssignable, Category = "Lobby")
    FOnTeamListChanged OnTeamListChanged;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void NotifyTeamChanged();
};