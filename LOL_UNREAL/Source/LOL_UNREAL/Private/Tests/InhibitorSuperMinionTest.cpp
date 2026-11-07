#if WITH_DEV_AUTOMATION_TESTS

#include "LOL_GameModeBase.h"
#include "Building/Building_Inhibitor.h"
#include "Component/LOL_StateComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Minion/Minion_Super.h"
#include "Minion/Minion_Siege.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInhibitorSuperMinionTest,
    "LOL.Minions.InhibitorSuperMinion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInhibitorSuperMinionTest::RunTest(const FString& Parameters)
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
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/LOL_UNREAL.LOL_GameModeBase"));
    if (!TestTrue(TEXT("Authority game mode installed"), World->SetGameMode(URL))) return false;
    ALOL_GameModeBase* Mode = World->GetAuthGameMode<ALOL_GameModeBase>();
    if (!TestNotNull(TEXT("Game mode"), Mode)) return false;
    TArray<AActor*>* Paths[] = {
        &Mode->BlueTopLanePoints, &Mode->BlueMidLanePoints, &Mode->BlueBotLanePoints,
        &Mode->RedTopLanePoints, &Mode->RedMidLanePoints, &Mode->RedBotLanePoints};
    for (int32 Lane = 0; Lane < 3; ++Lane)
    {
        AActor* BluePoint = World->SpawnActor<ATargetPoint>(FVector(0, Lane * 2000, 0), FRotator::ZeroRotator);
        AActor* RedPoint = World->SpawnActor<ATargetPoint>(FVector(10000, Lane * 2000, 0), FRotator::ZeroRotator);
        *Paths[Lane] = {BluePoint, RedPoint};
        *Paths[Lane + 3] = {RedPoint, BluePoint};
    }

    auto SpawnInhibitor = [World](bool bBlue, int32 Lane)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ABuilding_Inhibitor* Inhibitor = World->SpawnActor<ABuilding_Inhibitor>(
            FVector(bBlue ? 1000 : 9000, Lane * 2000, 500), FRotator::ZeroRotator, SpawnParams);
        Inhibitor->FindComponentByClass<ULOL_StateComponent>()->AddStatusTag(
            bBlue ? LOLTags::Team_Blue : LOLTags::Team_Red);
        return Inhibitor;
    };
    auto SpawnWave = [Mode]()
    {
        for (int32 Slot = 0; Slot < Mode->CurrentWaveMinions.Num(); ++Slot) Mode->SpawnNextMinion();
    };
    auto CountMinions = [World](UClass* Class)
    {
        int32 Count = 0;
        for (TActorIterator<ABaseMinion> It(World); It; ++It)
        {
            if (It->IsA(Class)) ++Count;
        }
        return Count;
    };
    auto ClearMinions = [World]()
    {
        for (TActorIterator<ABaseMinion> It(World); It; ++It) It->Destroy();
    };

    Mode->StartMinionWave();
    ABuilding_Inhibitor* RedMid = SpawnInhibitor(false, 1);
    RedMid->OnBuildingDeath();
    RedMid->OnBuildingDeath();
    TestEqual(TEXT("Repeated death notification is idempotent"), Mode->SuperMinionLanes.Num(), 1);
    TestTrue(TEXT("Red mid destruction empowers blue mid"), Mode->SuperMinionLanes.Contains(1));
    SpawnWave();
    TestEqual(TEXT("Destruction mid-wave does not change that wave"), CountMinions(AMinion_Super::StaticClass()), 0);
    TestEqual(TEXT("First wave has six normal minions per lane"), CountMinions(ABaseMinion::StaticClass()), 36);
    ClearMinions();
    RedMid->Destroy();

    Mode->StartMinionWave();
    SpawnWave();
    TestEqual(TEXT("Next non-siege wave has one super even after inhibitor despawns"), CountMinions(AMinion_Super::StaticClass()), 1);
    TestEqual(TEXT("Other lanes retain their normal wave"), CountMinions(ABaseMinion::StaticClass()), 37);
    for (TActorIterator<AMinion_Super> It(World); It; ++It)
    {
        TestTrue(TEXT("Super belongs to blue"), It->StateComponent->HasStatusTag(LOLTags::Team_Blue));
        TestTrue(TEXT("Super follows mid lane"), It->PathPoints.Num() == 2 && It->PathPoints[0].Y == 2000.f);
    }
    ClearMinions();

    Mode->StartMinionWave();
    SpawnWave();
    TestEqual(TEXT("Siege wave retains one super"), CountMinions(AMinion_Super::StaticClass()), 1);
    TestEqual(TEXT("Super replaces siege only in its own lane"), CountMinions(AMinion_Siege::StaticClass()), 5);
    ClearMinions();

    for (int32 Team = 0; Team < 2; ++Team)
    {
        for (int32 Lane = 0; Lane < 3; ++Lane)
        {
            Mode->SuperMinionLanes.Reset();
            ABuilding_Inhibitor* Inhibitor = SpawnInhibitor(Team == 0, Lane);
            Mode->NotifyInhibitorDestroyed(Inhibitor);
            TestTrue(TEXT("Every team/lane maps to the opposing spawn lane"),
                Mode->SuperMinionLanes.Num() == 1 && Mode->SuperMinionLanes.Contains(Lane + (Team == 0 ? 3 : 0)));
            Inhibitor->Destroy();
        }
    }

    Mode->SuperMinionLanes.Reset();
    ABuilding_Inhibitor* TaggedInhibitor = SpawnInhibitor(true, 0);
    TaggedInhibitor->Tags.Add(TEXT("Bot"));
    Mode->NotifyInhibitorDestroyed(TaggedInhibitor);
    TestTrue(TEXT("Explicit lane tag overrides proximity"), Mode->SuperMinionLanes.Contains(5));
    Mode->NotifyInhibitorDestroyed(SpawnInhibitor(false, 0));
    Mode->StartMinionWave();
    SpawnWave();
    TestEqual(TEXT("Opposing teams can have super minions simultaneously"), CountMinions(AMinion_Super::StaticClass()), 2);

    // Verify the actual map blueprints supply team tags before their death callback.
    const TCHAR* BlueprintPaths[] = {
        TEXT("/Game/Level/inhibitor/Blue_Inhibitor.Blue_Inhibitor_C"),
        TEXT("/Game/Level/inhibitor/Red_Inhibitor.Red_Inhibitor_C")};
    for (int32 Team = 0; Team < 2; ++Team)
    {
        UClass* Class = LoadClass<ABuilding_Inhibitor>(nullptr, BlueprintPaths[Team]);
        if (!TestNotNull(TEXT("Map inhibitor blueprint"), Class)) return false;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ABuilding_Inhibitor* Inhibitor = World->SpawnActor<ABuilding_Inhibitor>(
            Class, FVector(Team == 0 ? 1000 : 9000, 2000, 0), FRotator::ZeroRotator, SpawnParams);
        Inhibitor->DispatchBeginPlay();
        Mode->SuperMinionLanes.Reset();
        Inhibitor->OnBuildingDeath();
        TestTrue(TEXT("Map blueprint death enables the correct opposing lane"),
            Mode->SuperMinionLanes.Num() == 1 && Mode->SuperMinionLanes.Contains(Team == 0 ? 4 : 1));
    }
    return true;
}

#endif
