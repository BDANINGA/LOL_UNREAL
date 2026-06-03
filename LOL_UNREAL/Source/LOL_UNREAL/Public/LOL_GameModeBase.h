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

    // 게임 시작 시 호출되는 함수
    virtual void BeginPlay() override;
protected:
    UPROPERTY()
    TSubclassOf<APawn> AlistarClass;

    UPROPERTY()
    TSubclassOf<APawn> VayneClass;

    void StartMinionWave();

    UFUNCTION()
    void SpawnNextMinion();

    FTimerHandle MinionSpawnTimerHandle;

    UPROPERTY()
    TArray<TSubclassOf<class ABaseMinion>> CurrentWaveMinions;

    int32 SpawnedMinionCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
    AActor* BlueTeamSpawnPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
    AActor* RedTeamSpawnPoint;
};
