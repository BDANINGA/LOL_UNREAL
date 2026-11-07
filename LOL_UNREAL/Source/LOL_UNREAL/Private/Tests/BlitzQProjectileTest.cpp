#if WITH_DEV_AUTOMATION_TESTS

#include "Champion/Champion_Blitz.h"
#include "Champion/Projectile/BlitzQProjectile.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlitzQProjectileTest,
    "LOL.Combat.Blitz.QProjectileCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBlitzQProjectileTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    for (int32 Scenario = 0; Scenario < 4; ++Scenario)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(TEXT("Test world"), World)) return false;
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        ON_SCOPE_EXIT
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        };
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AChampion_Blitz* Blitz = World->SpawnActor<AChampion_Blitz>(
            FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (!TestNotNull(TEXT("Blitz"), Blitz)) return false;
        Blitz->StateComponent->AddStatusTag(LOLTags::Team_Blue);

        auto SpawnTarget = [&](FVector Position, bool bEnemy = true)
        {
            ABaseChampion* Target = World->SpawnActor<ABaseChampion>(Position, FRotator::ZeroRotator, SpawnParams);
            if (Target)
            {
                Target->StateComponent->AddStatusTag(bEnemy ? LOLTags::Team_Red : LOLTags::Team_Blue);
                Target->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            }
            return Target;
        };
        ABlitzQProjectile* Hook = World->SpawnActor<ABlitzQProjectile>(
            FVector(0.f, 0.f, 80.f), FRotator::ZeroRotator, SpawnParams);
        if (!TestNotNull(TEXT("Hook"), Hook)) return false;
        Hook->Initialize(Blitz, 1200.f, 1150.f, 50.f, nullptr, 30.f, 1.f, FRotator::ZeroRotator);

        if (Scenario == 0)
        {
            ABaseChampion* Target = SpawnTarget(FVector(600.f, 0.f, 0.f));
            if (!TestNotNull(TEXT("Dodge target"), Target)) return false;
            Hook->Tick(0.1f);
            TestEqual(TEXT("Hook flies at 1200 units per second"), Hook->GetActorLocation().X, 120.0);
            TestEqual(TEXT("A target ahead is not hit before the hook arrives"),
                Target->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
            Target->SetActorLocation(FVector(600.f, 400.f, 0.f));
            Hook->Tick(1.f);
            TestEqual(TEXT("Moving out of the path dodges the hook"),
                Target->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
            TestTrue(TEXT("A missed hook expires at maximum range"), Hook->IsActorBeingDestroyed());
        }
        else if (Scenario == 1)
        {
            ABaseChampion* Target = SpawnTarget(FVector(600.f, 400.f, 0.f));
            if (!TestNotNull(TEXT("Entering target"), Target)) return false;
            Target->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
            Hook->Tick(0.1f);
            Target->SetActorLocation(FVector(600.f, 0.f, 0.f));
            Hook->Tick(0.5f);
            TestEqual(TEXT("A Pawn entering the path after launch is grabbed"),
                Target->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
            TestTrue(TEXT("The projectile is removed on impact"), Hook->IsActorBeingDestroyed());
        }
        else if (Scenario == 2)
        {
            ABaseChampion* Ally = SpawnTarget(FVector(200.f, 0.f, 0.f), false);
            ABaseChampion* Dead = SpawnTarget(FVector(350.f, 0.f, 0.f));
            ABaseChampion* Near = SpawnTarget(FVector(600.f, 0.f, 0.f));
            ABaseChampion* Far = SpawnTarget(FVector(900.f, 0.f, 0.f));
            if (!Ally || !Dead || !Near || !Far) return false;
            Dead->StateComponent->AddStatusTag(LOLTags::State_Dead);
            Hook->Tick(1.f);
            TestEqual(TEXT("A long frame still hits the nearest live enemy champion"),
                Near->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
            TestEqual(TEXT("Allies are ignored"), Ally->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
            TestEqual(TEXT("Dead units are ignored"), Dead->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
            Hook->Tick(1.f);
            TestEqual(TEXT("Only one enemy can be grabbed per projectile"),
                Far->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
        }
        else
        {
            ABaseChampion* Target = SpawnTarget(FVector(1300.f, 0.f, 0.f));
            if (!TestNotNull(TEXT("Out of range target"), Target)) return false;
            Hook->Tick(3.f);
            TestEqual(TEXT("Long frames do not extend hook travel past its range"), Hook->GetActorLocation().X, 1150.0);
            TestEqual(TEXT("A body beyond the endpoint and hook radius is not hit"),
                Target->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
        }
    }
    return true;
}

#endif
