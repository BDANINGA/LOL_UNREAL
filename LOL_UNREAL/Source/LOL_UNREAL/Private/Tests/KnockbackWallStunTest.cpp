#if WITH_DEV_AUTOMATION_TESTS

#include "BaseChampion.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackWallStunTest,
    "LOL.Combat.Knockback.WallStun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockbackWallStunTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ABaseChampion* Target = World->SpawnActor<ABaseChampion>();
    if (!TestNotNull(TEXT("Target"), Target)) return false;

    for (float Duration : { 0.f, -1.f })
    {
        Target->ApplyStun(Duration);
        TestFalse(TEXT("Non-positive duration does not lock movement"), Target->bIsStunned);
        TestFalse(TEXT("Non-positive duration does not create a stun timer"),
            World->GetTimerManager().TimerExists(Target->StunHandle));
    }

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall"), Wall)) return false;
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->AddInstanceComponent(Box);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(20.f, 300.f, 300.f));
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(120.f, 0.f, 0.f));

    // Gragas E/R request knockback with no extra wall stun.
    Target->StartKnockbackWithWallCheck(FVector(500.f, 0.f, 80.f), 0.3f, 0.f);
    Target->CheckKnockbackWall();
    TestFalse(TEXT("Wall stops knockback"), Target->bIsKnockedBack);
    TestFalse(TEXT("Zero wall stun leaves movement unlocked"), Target->bIsStunned);

    // A normal E stun must retain its expiry when followed by a zero wall stun.
    Target->ApplyStun(1.f);
    Target->ApplyStun(0.f);
    Target->ApplyStun(-1.f);
    Target->StartKnockbackWithWallCheck(FVector(500.f, 0.f, 80.f), 0.3f, 0.f);
    Target->CheckKnockbackWall();
    TestTrue(TEXT("Existing stun remains active"), Target->bIsStunned);
    TestTrue(TEXT("Existing stun keeps its expiry timer"),
        World->GetTimerManager().TimerExists(Target->StunHandle));

    // Advance only this world's timers, without starting gameplay or physics.
    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    ++GFrameCounter;
    World->GetTimerManager().Tick(0.01f);
    ++GFrameCounter;
    World->GetTimerManager().Tick(1.1f);
    TestFalse(TEXT("Existing stun expires after the wall hit"), Target->bIsStunned);

    Target->StartKnockbackWithWallCheck(FVector(500.f, 0.f, 80.f), 0.3f, 0.5f);
    Target->CheckKnockbackWall();
    TestFalse(TEXT("Positive wall stun also ends knockback"), Target->bIsKnockedBack);
    TestTrue(TEXT("Abilities requesting a wall stun still apply it"), Target->bIsStunned);
    ++GFrameCounter;
    World->GetTimerManager().Tick(0.6f);
    TestFalse(TEXT("Positive wall stun expires normally"), Target->bIsStunned);
    return true;
}

#endif
