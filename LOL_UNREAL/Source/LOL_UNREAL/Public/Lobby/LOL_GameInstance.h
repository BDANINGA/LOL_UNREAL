#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LOL_GameInstance.generated.h"

UENUM(BlueprintType)
enum class EChampionID : uint8
{
    None          UMETA(DisplayName = "None"),
    Alistar       UMETA(DisplayName = "Alistar"),
    Blitzcrank    UMETA(DisplayName = "Blitzcrank"),
    Ezreal        UMETA(DisplayName = "Ezreal"),
    Fizz          UMETA(DisplayName = "Fizz"),
    Garen         UMETA(DisplayName = "Garen"),
    Gragas        UMETA(DisplayName = "Gragas"),
    Jax           UMETA(DisplayName = "Jax"),
    LeeSin        UMETA(DisplayName = "LeeSin"),
    Olaf          UMETA(DisplayName = "Tryndamier"),
    Vayne         UMETA(DisplayName = "Vayne")
};

UCLASS()
class LOL_UNREAL_API ULOL_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    UPROPERTY(BlueprintReadWrite, Category = "Player Data")
    FString MySavedNickname = "test";

    UPROPERTY(BlueprintReadWrite, Category = "Player Data")
    EChampionID MySelectedChampion = EChampionID::None;

    UPROPERTY(BlueprintReadWrite, Category = "Player Data")
    uint8 MySavedTeamID = 0;

    void BeginLobbyJoin(const FString& Address);
    void CompleteLobbyJoin();

private:
    void HandleTravelFailure(
        UWorld* World,
        ETravelFailure::Type FailureType,
        const FString& ErrorString);
    void HandleNetworkFailure(
        UWorld* World,
        class UNetDriver* NetDriver,
        ENetworkFailure::Type FailureType,
        const FString& ErrorString);
    void LogLobbyJoinFailure(const FString& FailureType, const FString& ErrorString);

    bool bLobbyJoinPending = false;
    FString PendingLobbyAddress;
};
