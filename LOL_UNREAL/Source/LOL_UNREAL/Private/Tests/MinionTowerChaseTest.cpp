#if WITH_DEV_AUTOMATION_TESTS

#include "Minion/Minion_Melee.h"
#include "Minion/Minion_Caster.h"
#include "Minion/Minion_Siege.h"
#include "Minion/LOL_MinionAIController.h"
#include "Building/Building_Turret.h"
#include "Building/LOL_TurretAIController.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinionTowerChaseTest,
    "LOL.Minions.TowerChase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinionTowerChaseTest::RunTest(const FString& Parameters)
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
    // Pawn damage is rejected by the engine unless an authority game mode exists.
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    if (!TestTrue(TEXT("Authority game mode"), World->SetGameMode(URL))) return false;

    AMinion_Melee* Minion = World->SpawnActor<AMinion_Melee>();
    ABuilding_Turret* Tower = World->SpawnActor<ABuilding_Turret>(
        FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator);
    ALOL_MinionAIController* Controller = World->SpawnActor<ALOL_MinionAIController>();
    if (!TestNotNull(TEXT("Minion"), Minion) || !TestNotNull(TEXT("Tower"), Tower) ||
        !TestNotNull(TEXT("Controller"), Controller)) return false;
    Controller->Possess(Minion);
    Minion->DispatchBeginPlay();
    Tower->DispatchBeginPlay();
    ULOL_StateComponent* TowerState = Tower->FindComponentByClass<ULOL_StateComponent>();
    ULOL_StatComponent* TowerStat = Tower->FindComponentByClass<ULOL_StatComponent>();
    Minion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
    TowerState->AddStatusTag(LOLTags::Team_Red);

    // Install a real path-follow request that would take the minion past the tower.
    const FVector LaneDestination(0.f, 2000.f, 0.f);
    Minion->PathPoints = { LaneDestination };
    Minion->CurrentPathIndex = 0;
    Minion->MoveComponent->SetMoveTarget(LaneDestination, nullptr);
    FNavPathSharedPtr LanePath = MakeShared<FNavigationPath, ESPMode::ThreadSafe>();
    LanePath->GetPathPoints().Add(FNavPathPoint(Minion->GetActorLocation()));
    LanePath->GetPathPoints().Add(FNavPathPoint(LaneDestination));
    LanePath->MarkReady();
    FAIMoveRequest LaneRequest(LaneDestination);
    LaneRequest.SetAcceptanceRadius(15.f);
    Controller->GetPathFollowingComponent()->RequestMove(LaneRequest, LanePath);
    TestTrue(TEXT("Lane path is active before combat"), Controller->GetMoveStatus() == EPathFollowingStatus::Moving);
    Minion->AddMovementInput(FVector::RightVector, 1.f, true);

    // Advance AI decisions without starting a full match.
    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    auto AdvanceTimers = [World](float Seconds)
    {
        ++GFrameCounter;
        World->GetTimerManager().Tick(Seconds);
    };
    AdvanceTimers(0.01f);
    AdvanceTimers(0.21f);
    TestTrue(TEXT("AI acquires the enemy tower"), Minion->AttackComponent->CombatTarget == Tower);
    TestTrue(TEXT("Acquiring the tower cancels the old lane path"), Controller->GetMoveStatus() == EPathFollowingStatus::Idle);
    TestTrue(TEXT("Old lane input is discarded"), Minion->GetPendingMovementInputVector().IsNearlyZero());

    Minion->AttackComponent->UpdateAttackLogic();
    Minion->MoveComponent->UpdateMovement(0.016f);
    TestTrue(TEXT("Chase destination is the tower"), Minion->MoveComponent->TargetLocation.Equals(Tower->GetActorLocation()));
    TestTrue(TEXT("Without a navmesh the fallback input points toward the tower"),
        FVector::DotProduct(Minion->GetPendingMovementInputVector().GetSafeNormal(), FVector::ForwardVector) > 0.99f);
    TestEqual(TEXT("Combat does not advance the lane waypoint"), Minion->CurrentPathIndex, 0);

    // A ground-level tower pivot must not keep a melee minion outside its horizontal range.
    const float TowerRadius = Tower->FindComponentByClass<UCapsuleComponent>()->GetScaledCapsuleRadius();
    const float AttackRange = Minion->StatComponent->GetStat().AttackRange;
    Minion->SetActorLocation(Tower->GetActorLocation() + FVector(-(TowerRadius + AttackRange - 5.f), 0.f, 96.f));
    Minion->AttackComponent->UpdateAttackLogic();
    TestTrue(TEXT("Melee attack starts at the tower edge"), Minion->AttackComponent->HitTarget == Tower);
    TestTrue(TEXT("Chase input is cleared when attacking"), Minion->GetPendingMovementInputVector().IsNearlyZero());
    const float BeforeHP = TowerStat->GetCurrentHP();
    TestTrue(TEXT("Tower begins with health"), BeforeHP > 0.f);
    Minion->AttackComponent->ExecuteAttackHit();
    TestTrue(TEXT("Melee hit damages the tower"), TowerStat->GetCurrentHP() < BeforeHP);

    ABaseJungleMonster* Monster = World->SpawnActor<ABaseJungleMonster>(FVector(100.f, 150.f, 0.f), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Monster"), Monster)) return false;
    Monster->StateComponent->AddStatusTag(LOLTags::Team_Jungle);
    TestFalse(TEXT("Jungle monsters remain excluded"), Minion->AttackComponent->IsValidAttackTarget(Monster));
    TowerState->AddStatusTag(LOLTags::State_Dead);
    AdvanceTimers(0.21f);
    TestNull(TEXT("Destroyed tower is released"), Minion->AttackComponent->CombatTarget);
    TestTrue(TEXT("AI resumes the original lane destination"), Minion->MoveComponent->TargetLocation.Equals(LaneDestination));
    TestEqual(TEXT("Lane resumes from the same waypoint"), Minion->CurrentPathIndex, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurretRangedMinionRangeTest,
    "LOL.Minions.TurretRangedRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTurretRangedMinionRangeTest::RunTest(const FString& Parameters)
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
    URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    if (!TestTrue(TEXT("Authority game mode"), World->SetGameMode(URL))) return false;

    ABuilding_Turret* Tower = World->SpawnActor<ABuilding_Turret>();
    ALOL_TurretAIController* TowerAI = World->SpawnActor<ALOL_TurretAIController>();
    if (!Tower || !TowerAI) return false;
    TowerAI->Possess(Tower);
    Tower->DispatchBeginPlay();
    Tower->FindComponentByClass<UCapsuleComponent>()->SetCapsuleRadius(94.5f);
    Tower->FindComponentByClass<ULOL_StateComponent>()->AddStatusTag(LOLTags::Team_Red);
    ULOL_StatComponent* TowerStats = Tower->FindComponentByClass<ULOL_StatComponent>();
    ULOL_AttackComponent* TowerAttack = Tower->FindComponentByClass<ULOL_AttackComponent>();
    TestEqual(TEXT("Saved turret range remains 580"), TowerStats->GetStat().AttackRange, 580.f);

    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    auto AdvanceTimers = [World](float Seconds)
    {
        ++GFrameCounter;
        World->GetTimerManager().Tick(Seconds);
    };
    AdvanceTimers(0.01f);
    for (UClass* MinionClass : { AMinion_Caster::StaticClass(), AMinion_Siege::StaticClass() })
    {
        ABaseMinion* Minion = World->SpawnActor<ABaseMinion>(MinionClass, FVector(600.f, 0.f, 96.f), FRotator::ZeroRotator);
        ALOL_MinionAIController* MinionAI = World->SpawnActor<ALOL_MinionAIController>();
        if (!Minion || !MinionAI) return false;
        MinionAI->Possess(Minion);
        Minion->DispatchBeginPlay();
        Minion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
        Minion->AttackComponent->SetCombatTarget(Tower);
        const float Range = Minion->StatComponent->GetStat().AttackRange;
        if (MinionClass == AMinion_Caster::StaticClass())
            TestEqual(TEXT("Caster range remains 550"), Range, 550.f);

        TowerAttack->bCanAttack = true;
        AdvanceTimers(0.21f);
        TestNull(TEXT("Capsule overlap outside 580 does not start a turret attack"), TowerAttack->HitTarget);
        TestNull(TEXT("Turret does not acquire a center outside 580"), TowerAttack->CombatTarget);

        // Previously this position was in range after subtracting the tower radius.
        Minion->SetActorLocation(FVector(Range + 50.f, 0.f, 96.f));
        Minion->AttackComponent->UpdateAttackLogic();
        Minion->MoveComponent->UpdateMovement(0.016f);
        TestNull(TEXT("Ranged minion cannot shoot from the old edge-based range"), Minion->AttackComponent->HitTarget);
        TestTrue(TEXT("Out-of-range minion keeps approaching the tower"), Minion->GetPendingMovementInputVector().X < 0.f);

        Minion->SetActorLocation(FVector(Range - 1.f, 0.f, 96.f));
        Minion->AttackComponent->UpdateAttackLogic();
        TestNull(TEXT("Height difference counts toward ranged turret reach"), Minion->AttackComponent->HitTarget);

        const float HorizontalReach = FMath::Sqrt(FMath::Square(Range) - FMath::Square(96.f));
        Minion->SetActorLocation(FVector(HorizontalReach - 1.f, 0.f, 96.f));
        Minion->AttackComponent->UpdateAttackLogic();
        TestTrue(TEXT("Ranged minion starts firing inside its center range"), Minion->AttackComponent->HitTarget == Tower);
        TestTrue(TEXT("Minion stops approaching when it can attack"), Minion->GetPendingMovementInputVector().IsNearlyZero());
        AdvanceTimers(0.21f);
        TestTrue(TEXT("Turret can target the attacking ranged minion"), TowerAttack->CombatTarget == Minion);
        TestTrue(TEXT("Turret starts its return attack"), TowerAttack->HitTarget == Minion);

        Minion->AttackComponent->ResetAfterRespawn();
        Minion->StateComponent->AddStatusTag(LOLTags::State_Dead);
        Minion->SetActorEnableCollision(false);
        TowerAttack->ResetAfterRespawn();
    }
    return true;
}

#endif
