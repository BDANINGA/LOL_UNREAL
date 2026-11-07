#if WITH_DEV_AUTOMATION_TESTS

#include "BaseChampion.h"
#include "Component/LOL_FixedKnockbackComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFixedKnockbackTest,
    "LOL.Combat.Vayne.FixedKnockback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFixedKnockbackTest::RunTest(const FString& Parameters)
{
    // This isolated world does not run BeginPlay; allow the native multicast stun RPC.
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ABaseChampion* Target = World->SpawnActor<ABaseChampion>();
    if (!TestNotNull(TEXT("Target"), Target)) return false;
    ULOL_FixedKnockbackComponent* Knockback = NewObject<ULOL_FixedKnockbackComponent>(Target);
    Target->AddInstanceComponent(Knockback);
    Knockback->RegisterComponent();
    UCharacterMovementComponent* Movement = Target->GetCharacterMovement();

    for (float Step : { 0.016f, 0.13f, 1.f })
    {
        Target->SetActorLocation(FVector::ZeroVector);
        Movement->SetMovementMode(MOVE_Walking);
        Movement->Velocity = FVector(9000.f, 1000.f, 100.f);
        Knockback->StartKnockback(FVector::ForwardVector, 500.f, 0.4f, 5.f);
        for (float Time = 0.f; Time < 0.6f; Time += Step)
        {
            Knockback->TickComponent(Step, LEVELTICK_All, nullptr);
            TestTrue(TEXT("Never travels beyond 500 even on a long frame"), Target->GetActorLocation().X <= 500.01f);
        }
        TestTrue(TEXT("Unobstructed knockback ends exactly 500 units from its start"),
            Target->GetActorLocation().Equals(FVector(500.f, 0.f, 0.f), 0.01f));
        TestTrue(TEXT("No momentum remains after knockback"), Movement->Velocity.IsNearlyZero());
        TestFalse(TEXT("Knockback state cleared"), Target->bIsKnockedBack);
        TestFalse(TEXT("Completed movement no longer ticks"), Knockback->IsComponentTickEnabled());
    }

    AActor* Wall = World->SpawnActor<AActor>();
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->AddInstanceComponent(Box);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(20.f, 300.f, 300.f));
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(400.f, 0.f, 0.f));

    Target->SetActorLocation(FVector::ZeroVector);
    Knockback->StartKnockback(FVector::ForwardVector, 500.f, 0.4f, 5.f);
    Knockback->TickComponent(1.f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("A swept long frame stops before the wall"),
        Target->GetActorLocation().X > 0.f && Target->GetActorLocation().X < 400.f);
    TestTrue(TEXT("Wall collision still stuns a champion"), Target->bIsStunned);
    TestFalse(TEXT("Wall collision finishes knockback"), Target->bIsKnockedBack);
    const FVector WallStop = Target->GetActorLocation();
    Knockback->TickComponent(1.f, LEVELTICK_All, nullptr);
    TestTrue(TEXT("Cannot resume drifting after a wall hit"), Target->GetActorLocation().Equals(WallStop));
    return true;
}

#endif
