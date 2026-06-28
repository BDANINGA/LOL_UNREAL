#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GS_ChampSelect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerChanged, int32, NewTime);

UCLASS()
class LOL_UNREAL_API AGS_ChampSelect : public AGameStateBase
{
    GENERATED_BODY()

public:
    AGS_ChampSelect();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 남은 시간 (서버에서 깎고 클라이언트로 복제됨)
    UPROPERTY(ReplicatedUsing = OnRep_TimeLeft, BlueprintReadOnly, Category = "ChampSelect")
    int32 TimeLeft = 60;

    UPROPERTY(BlueprintAssignable, Category = "ChampSelect|Events")
    FOnTimerChanged OnTimerChanged;

protected:
    UFUNCTION()
    void OnRep_TimeLeft();
};