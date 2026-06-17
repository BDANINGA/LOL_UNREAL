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

    // 플레이어 컨트롤러에 따른 캐릭터 클래스 반환 함수
    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    void RequestRespawn(class ABaseChampion* DeadChampion);
    void RequestJungleMonsterRespawn(FName MonsterRowName, FVector SpawnLocation, FRotator SpawnRotation, float RespawnDelay);

    // 게임 시작 시 호출되는 함수
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

private:
    FTimerHandle MinionSpawnTimerHandle;

    UPROPERTY()
    TArray<TSubclassOf<class ABaseMinion>> CurrentWaveMinions;
    UPROPERTY(EditAnywhere, Category = "Spawn")
    int32 SpawnedMinionCount;

    TArray<AActor*> BlueTopLanePoints;
    TArray<AActor*> BlueMidLanePoints;
    TArray<AActor*> BlueBotLanePoints;
    TArray<AActor*> RedTopLanePoints;
    TArray<AActor*> RedMidLanePoints;
    TArray<AActor*> RedBotLanePoints;
};
