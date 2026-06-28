#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PC_ChampSelect.generated.h"

UCLASS()
class LOL_UNREAL_API APC_ChampSelect : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> ChampSelectWidgetClass;

    UPROPERTY()
    UUserWidget* ChampSelectWidget;

    // 챔피언 마우스 올림 요청 (Server RPC)
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ChampSelect")
    void Server_HoverChampion(EChampionID Champ);

    // 챔피언 선택 확정(준비 완료) 요청 (Server RPC)
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ChampSelect")
    void Server_LockInChampion();
};