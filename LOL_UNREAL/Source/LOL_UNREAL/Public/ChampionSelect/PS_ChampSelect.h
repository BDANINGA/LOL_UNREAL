#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/LOL_GameInstance.h"
#include "PS_ChampSelect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChampSelectDataChanged);

UCLASS()
class LOL_UNREAL_API APS_ChampSelect : public APlayerState
{
    GENERATED_BODY()

public:
    APS_ChampSelect();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 로비에서 받아올 데이터
    UPROPERTY(ReplicatedUsing = OnRep_DataChanged, BlueprintReadOnly, Category = "ChampSelect")
    FString Nickname;

    UPROPERTY(ReplicatedUsing = OnRep_DataChanged, BlueprintReadOnly, Category = "ChampSelect")
    uint8 TeamID = 0;

    // 픽창 전용 데이터
    UPROPERTY(ReplicatedUsing = OnRep_DataChanged, BlueprintReadOnly, Category = "ChampSelect")
    EChampionID HoveredChampion = EChampionID::None;

    UPROPERTY(ReplicatedUsing = OnRep_DataChanged, BlueprintReadOnly, Category = "ChampSelect")
    EChampionID LockedChampion = EChampionID::None;

    UPROPERTY(ReplicatedUsing = OnRep_DataChanged, BlueprintReadOnly, Category = "ChampSelect")
    bool bIsReady = false;

    // 데이터 복제 완료 시 UI를 리프레시할 이벤트
    UPROPERTY(BlueprintAssignable, Category = "ChampSelect|Events")
    FOnChampSelectDataChanged OnChampSelectDataChanged;

    virtual void CopyProperties(APlayerState* NewPlayerState) override;

protected:
    UFUNCTION()
    void OnRep_DataChanged();
};