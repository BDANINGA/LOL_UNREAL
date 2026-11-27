#if WITH_DEV_AUTOMATION_TESTS

#include "Component/LOL_LifeCycleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnCountdownTest,
    "LOL.HUD.RespawnCountdown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRespawnCountdownTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    APawn* Pawn = World->SpawnActor<APawn>();
    ULOL_LifeCycleComponent* LifeCycle = NewObject<ULOL_LifeCycleComponent>(Pawn);
    Pawn->AddInstanceComponent(LifeCycle);
    LifeCycle->RegisterComponent();
    Pawn->DispatchBeginPlay();
    AGameStateBase* GameState = World->SpawnActor<AGameStateBase>();
    World->SetGameState(GameState);

    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    auto Advance = [World](float Seconds)
    {
        // UWorld clamps long frames; advance in normal-sized steps so the
        // server clock and timer manager both experience the requested duration.
        while (Seconds > KINDA_SMALL_NUMBER)
        {
            const float Step = FMath::Min(Seconds, 0.1f);
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, Step);
            Seconds -= Step;
        }
    };
    Advance(0.01f);

    TestFalse(TEXT("Alive pawn has no countdown"), LifeCycle->HasPendingRespawn());
    LifeCycle->ScheduleRespawn(2.5f);
    TestTrue(TEXT("Respawn publishes a deadline"), LifeCycle->HasPendingRespawn());
    TestEqual(TEXT("Fractional initial time rounds up"), FMath::CeilToInt(LifeCycle->GetRespawnTimeRemaining()), 3);
    Advance(1.0f);
    TestTrue(TEXT("Countdown tracks elapsed server time"), FMath::IsNearlyEqual(LifeCycle->GetRespawnTimeRemaining(), 1.5f, 0.01f));

    LifeCycle->ScheduleRespawn(3.0f);
    Advance(1.6f);
    TestTrue(TEXT("Rescheduling cancels the previous respawn"), LifeCycle->HasPendingRespawn());
    Advance(1.5f);
    TestFalse(TEXT("Actual respawn clears the countdown"), LifeCycle->HasPendingRespawn());
    TestEqual(TEXT("Remaining time never goes negative"), LifeCycle->GetRespawnTimeRemaining(), 0.0f);

    LifeCycle->ScheduleRespawn(10.0f);
    LifeCycle->Respawn();
    TestFalse(TEXT("Early respawn clears the countdown"), LifeCycle->HasPendingRespawn());
    LifeCycle->bCanRespawn = false;
    LifeCycle->ScheduleRespawn(2.0f);
    TestFalse(TEXT("Non-respawning units have no countdown"), LifeCycle->HasPendingRespawn());
    return true;
}

#endif
