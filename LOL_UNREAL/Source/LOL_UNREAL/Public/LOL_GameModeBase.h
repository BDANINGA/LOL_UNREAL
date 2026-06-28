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
    
    void PostLogin(APlayerController* NewPlayer);

    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    void RequestRespawn(class ABaseChampion* DeadChampion);
    void RequestJungleMonsterRespawn(FName MonsterRowName, FVector SpawnLocation, FRotator SpawnRotation, float RespawnDelay);

    virtual void BeginPlay() override;
protected:
    UPROPERTY()
    TSubclassOf<APawn> AlistarClass;

    UPROPERTY()
    TSubclassOf<APawn> VayneClass;

    void StartMinionWave();
    void SpawnNextMinion();
    void SpawnJungleMonsters();
    void SpawnJungleMonsterAtTag(FName TargetTag, FName MonsterRowName);
    void SpawnJungleMonsterAtTransform(FName MonsterRowName, FVector SpawnLocation, FRotator SpawnRotation);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<AHUD> DefaultHUDClass;

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
