#include "Lobby/LOL_GameInstance.h"

#include "Engine/Engine.h"

void ULOL_GameInstance::Init()
{
    Super::Init();

    if (GEngine)
    {
        GEngine->OnTravelFailure().AddUObject(this, &ULOL_GameInstance::HandleTravelFailure);
        GEngine->OnNetworkFailure().AddUObject(this, &ULOL_GameInstance::HandleNetworkFailure);
    }
}

void ULOL_GameInstance::Shutdown()
{
    if (GEngine)
    {
        GEngine->OnTravelFailure().RemoveAll(this);
        GEngine->OnNetworkFailure().RemoveAll(this);
    }

    Super::Shutdown();
}

void ULOL_GameInstance::BeginLobbyJoin(const FString& Address)
{
    PendingLobbyAddress = Address;
    bLobbyJoinPending = true;
}

void ULOL_GameInstance::CompleteLobbyJoin()
{
    if (bLobbyJoinPending)
    {
        UE_LOG(LogTemp, Log, TEXT("Lobby join succeeded. Address=%s"), *PendingLobbyAddress);
    }

    PendingLobbyAddress.Empty();
    bLobbyJoinPending = false;
}

void ULOL_GameInstance::HandleTravelFailure(
    UWorld* World,
    ETravelFailure::Type FailureType,
    const FString& ErrorString)
{
    if (bLobbyJoinPending)
    {
        LogLobbyJoinFailure(ETravelFailure::ToString(FailureType), ErrorString);
    }
}

void ULOL_GameInstance::HandleNetworkFailure(
    UWorld* World,
    UNetDriver* NetDriver,
    ENetworkFailure::Type FailureType,
    const FString& ErrorString)
{
    if (bLobbyJoinPending)
    {
        LogLobbyJoinFailure(ENetworkFailure::ToString(FailureType), ErrorString);
    }
}

void ULOL_GameInstance::LogLobbyJoinFailure(
    const FString& FailureType,
    const FString& ErrorString)
{
    UE_LOG(
        LogTemp,
        Error,
        TEXT("Lobby join failed. Address=%s Type=%s Reason=%s"),
        *PendingLobbyAddress,
        *FailureType,
        ErrorString.IsEmpty() ? TEXT("No additional error was provided.") : *ErrorString);

    if (GEngine)
    {
        const FString ScreenMessage = FString::Printf(
            TEXT("Lobby join failed: %s"),
            ErrorString.IsEmpty() ? *FailureType : *ErrorString);
        GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, ScreenMessage);
    }

    PendingLobbyAddress.Empty();
    bLobbyJoinPending = false;
}

