#if WITH_DEV_AUTOMATION_TESTS

#include "LOL_GameModeBase.h"
#include "LOL_GameState.h"
#include "LOL_PlayerController.h"
#include "LOL_PlayerState.h"
#include "Building/Building_Nexus.h"
#include "Component/LOL_StatComponent.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"
#include "Widget/LOL_GameResultWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameResultTest,
    "LOL.Match.NexusGameResult",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameResultTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    World->SetGameInstance(Instance);
    FURL URL;
    URL.AddOption(TEXT("game=/Script/LOL_UNREAL.LOL_GameModeBase"));
    if (!TestTrue(TEXT("Game mode installed"), World->SetGameMode(URL))) return false;
    ALOL_GameModeBase* Mode = World->GetAuthGameMode<ALOL_GameModeBase>();

    ALOL_PlayerController* BluePlayer = World->SpawnActor<ALOL_PlayerController>();
    ALOL_PlayerController* RedPlayer = World->SpawnActor<ALOL_PlayerController>();
    BluePlayer->PlayerState = World->SpawnActor<ALOL_PlayerState>();
    RedPlayer->PlayerState = World->SpawnActor<ALOL_PlayerState>();
    BluePlayer->GetPlayerState<ALOL_PlayerState>()->TeamID = 1;
    RedPlayer->GetPlayerState<ALOL_PlayerState>()->TeamID = 2;

    const TCHAR* NexusClasses[] = {
        TEXT("/Game/Level/nexus/Blue_Nexus.Blue_Nexus_C"),
        TEXT("/Game/Level/nexus/Red_Nexus.Red_Nexus_C")};
    for (int32 LosingTeam = 1; LosingTeam <= 2; ++LosingTeam)
    {
        ALOL_GameState* State = World->SpawnActor<ALOL_GameState>();
        World->SetGameState(State);
        bool bVictory = true;
        TestFalse(TEXT("No result before a nexus dies"), BluePlayer->GetMatchVictory(bVictory));
        TestFalse(TEXT("Invalid winner rejected"), State->FinishMatch(3));
        Mode->StartMinionWave();
        TestTrue(TEXT("Wave timer initially running"), World->GetTimerManager().TimerExists(Mode->MinionSpawnTimerHandle));

        UClass* NexusClass = LoadClass<ABuilding_Nexus>(nullptr, NexusClasses[LosingTeam - 1]);
        if (!TestNotNull(TEXT("Actual nexus blueprint"), NexusClass)) return false;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ABuilding_Nexus* Nexus = World->SpawnActor<ABuilding_Nexus>(NexusClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        Nexus->DispatchBeginPlay();
        ULOL_StatComponent* Stats = Nexus->FindComponentByClass<ULOL_StatComponent>();
        if (!TestNotNull(TEXT("Nexus stats"), Stats)) return false;
        TestTrue(TEXT("Nexus starts alive"), Stats->GetCurrentHP() > 0.f);
        Stats->ApplyDamage(1000000.f, EDamageType::TrueDamage, nullptr, nullptr);

        const uint8 WinningTeam = LosingTeam == 1 ? 2 : 1;
        TestEqual(TEXT("Lethal nexus damage gives the other team victory"), State->GetWinningTeamID(), WinningTeam);
        TestFalse(TEXT("Wave timer stops at match end"), World->GetTimerManager().TimerExists(Mode->MinionSpawnTimerHandle));
        const int32 WavesBefore = Mode->MinionWaveCount;
        Mode->StartMinionWave();
        TestEqual(TEXT("No further waves after result"), Mode->MinionWaveCount, WavesBefore);
        const float MatchTime = State->CurrentMatchTime;
        State->TickOneSecond();
        TestEqual(TEXT("Match clock stops"), State->CurrentMatchTime, MatchTime);

        TestTrue(TEXT("Blue player can read result"), BluePlayer->GetMatchVictory(bVictory));
        TestEqual(TEXT("Blue player's result"), bVictory, WinningTeam == 1);
        TestTrue(TEXT("Red player can read result"), RedPlayer->GetMatchVictory(bVictory));
        TestEqual(TEXT("Red player's result"), bVictory, WinningTeam == 2);
        // No pawn is required: dead players must also see their team's result.
        TestNull(TEXT("Result works without a possessed champion"), BluePlayer->GetPawn());

        UClass* OtherClass = LoadClass<ABuilding_Nexus>(nullptr, NexusClasses[WinningTeam - 1]);
        ABuilding_Nexus* OtherNexus = World->SpawnActor<ABuilding_Nexus>(OtherClass, FVector(1000, 0, 0), FRotator::ZeroRotator, SpawnParams);
        OtherNexus->DispatchBeginPlay();
        OtherNexus->FindComponentByClass<ULOL_StatComponent>()->ApplyDamage(1000000.f, EDamageType::TrueDamage, nullptr, nullptr);
        TestEqual(TEXT("Second nexus death cannot overwrite first result"), State->GetWinningTeamID(), WinningTeam);
        TestFalse(TEXT("Winner cannot be overwritten directly"), State->FinishMatch(LosingTeam));
        Nexus->Destroy();
        OtherNexus->Destroy();
    }

    ULOL_GameResultWidget* Widget = NewObject<ULOL_GameResultWidget>(Instance);
    ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    LocalPlayer->PlayerController = BluePlayer;
    BluePlayer->Player = LocalPlayer;
    World->AddController(BluePlayer);
    Widget->SetPlayerContext(FLocalPlayerContext(LocalPlayer, World));
    TestTrue(TEXT("Result widget initializes"), Widget->Initialize());
    UImage* Image = Cast<UImage>(Widget->GetWidgetFromName(TEXT("ResultImage")));
    if (!TestNotNull(TEXT("Result image exists"), Image)) return false;
    Widget->SetVictory(true);
    UTexture2D* Victory = Cast<UTexture2D>(Image->GetBrush().GetResourceObject());
    if (!TestNotNull(TEXT("Victory texture loaded"), Victory)) return false;
    TestEqual(TEXT("Uses provided victory texture"), Victory->GetPathName(), FString(TEXT("/Game/UI/victory_defeat/victory.victory")));
    Widget->SetVictory(false);
    UTexture2D* Defeat = Cast<UTexture2D>(Image->GetBrush().GetResourceObject());
    if (!TestNotNull(TEXT("Defeat texture loaded"), Defeat)) return false;
    TestEqual(TEXT("Uses provided defeat texture"), Defeat->GetPathName(), FString(TEXT("/Game/UI/victory_defeat/defeat.defeat")));
    TestTrue(TEXT("Preserves texture aspect ratio"), Image->GetBrush().ImageSize.Equals(FVector2D(Defeat->GetSizeX(), Defeat->GetSizeY())));
    return true;
}

#endif
