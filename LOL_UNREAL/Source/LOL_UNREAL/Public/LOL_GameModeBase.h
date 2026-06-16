#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LOL_GameModeBase.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_GameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
    ALOL_GameModeBase();

    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    void RequestRespawn(class ABaseChampion* DeadChampion);

    virtual void BeginPlay() override;
protected:
    UPROPERTY()
    TSubclassOf<APawn> AlistarClass;

    UPROPERTY()
    TSubclassOf<APawn> VayneClass;

    void StartMinionWave();
    void SpawnNextMinion();

private:
    FTimerHandle MinionSpawnTimerHandle;

    UPROPERTY()
    TArray<TSubclassOf<class ABaseMinion>> CurrentWaveMinions;

    int32 SpawnedMinionCount{};
    int32 MinionWaveCount{};

    TArray<AActor*> BlueTopLanePoints;
    TArray<AActor*> BlueMidLanePoints;
    TArray<AActor*> BlueBotLanePoints;
    TArray<AActor*> RedTopLanePoints;
    TArray<AActor*> RedMidLanePoints;
    TArray<AActor*> RedBotLanePoints;
};
