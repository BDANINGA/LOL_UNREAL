#if WITH_DEV_AUTOMATION_TESTS

#include "Champion/Champion_Blitz.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlitzQPullReleaseTest,
    "LOL.Combat.Blitz.QPullRelease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBlitzQPullReleaseTest::RunTest(const FString& Parameters)
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
    AChampion_Blitz* Blitz = World->SpawnActor<AChampion_Blitz>();
    ABaseChampion* Target = World->SpawnActor<ABaseChampion>(FVector(600.f, 0.f, 0.f), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Blitz"), Blitz) || !TestNotNull(TEXT("Target"), Target)) return false;
    Blitz->StateComponent->AddStatusTag(LOLTags::Team_Blue);
    Target->StateComponent->AddStatusTag(LOLTags::Team_Red);
    Target->SetActorScale3D(FVector(1.5f));
    Target->MoveComponent->StopMovement();
    Target->MoveComponent->TargetLocation = FVector(2000.f, 0.f, 0.f);
    Target->MoveComponent->bIsSearchAttack = true;
    Target->StateComponent->AddStatusTag(LOLTags::State_Moving);
    Target->AddMovementInput(FVector::ForwardVector, 1.f, true);
    UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
    Movement->SetMovementMode(MOVE_Walking);
    Movement->Velocity = FVector(800.f, 0.f, 0.f);
    const ECollisionEnabled::Type CollisionBefore = Target->GetCapsuleComponent()->GetCollisionEnabled();

    Blitz->BeginPullTarget(Target);
    TestTrue(TEXT("Grab locks new movement commands"), Target->IsMoveInputBlocked());
    TestFalse(TEXT("Old move order is cleared"), Target->StateComponent->HasStatusTag(LOLTags::State_Moving));
    TestFalse(TEXT("Attack-move is cleared"), Target->MoveComponent->bIsSearchAttack);
    TestTrue(TEXT("Pending movement input is cleared"), Target->GetPendingMovementInputVector().IsNearlyZero());
    Target->MoveComponent->SetMoveTarget(FVector(3000.f, 0.f, 0.f), nullptr);
    TestTrue(TEXT("A new path cannot be queued while grabbed"),
        Target->MoveComponent->TargetLocation.Equals(FVector(600.f, 0.f, 0.f)));

    // Blitz walks forward while the target is still being pulled.
    Blitz->SetActorLocation(FVector(100.f, 0.f, 0.f));
    for (int32 Step = 0; Step < 80 && Blitz->GrabbedTarget; ++Step) Blitz->TickPullTarget();
    const float MinimumSeparation = Blitz->GetCapsuleComponent()->GetScaledCapsuleRadius() +
        Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
    TestNull(TEXT("Pull completes"), Blitz->GrabbedTarget);
    TestTrue(TEXT("Scaled capsule bodies do not overlap at release"),
        FVector::Dist2D(Blitz->GetActorLocation(), Target->GetActorLocation()) >= MinimumSeparation + 5.f);
    TestFalse(TEXT("Input lock is released"), Target->IsGrabbed());
    TestTrue(TEXT("No leftover velocity"), Movement->Velocity.IsNearlyZero());
    TestTrue(TEXT("Old destination is replaced by the release position"),
        Target->MoveComponent->TargetLocation.Equals(Target->GetActorLocation()));
    TestEqual(TEXT("Original movement mode is restored"), Movement->MovementMode.GetValue(), MOVE_Walking);
    TestEqual(TEXT("Original collision is restored"), Target->GetCapsuleComponent()->GetCollisionEnabled(), CollisionBefore);
    TestFalse(TEXT("No stale timeout remains after arriving"), World->GetTimerManager().TimerExists(Blitz->PullTimeoutTimerHandle));
    const FVector ReleaseLocation = Target->GetActorLocation();
    for (int32 Step = 0; Step < 100; ++Step) Target->MoveComponent->UpdateMovement(0.01f);
    TestTrue(TEXT("Released target does not resume its previous path"),
        Target->GetPendingMovementInputVector().IsNearlyZero() && Target->GetActorLocation().Equals(ReleaseLocation));

    // Normal forward walking must block against the target, not start inside its body.
    FHitResult Hit;
    Blitz->SetActorLocation(Target->GetActorLocation(), true, &Hit);
    TestTrue(TEXT("Walking forward is blocked by the released target"), Hit.bBlockingHit);
    TestFalse(TEXT("Forward walking does not start in penetration"), Hit.bStartPenetrating);
    TestTrue(TEXT("Forward walking does not relocate the target"), Target->GetActorLocation().Equals(ReleaseLocation));

    // An interrupted pull must enforce separation and clear both timers too.
    Target->SetActorLocation(Blitz->GetActorLocation() + FVector(500.f, 0.f, 0.f));
    Blitz->BeginPullTarget(Target);
    Target->SetActorLocation(Blitz->GetActorLocation() + FVector(5.f, 0.f, 0.f));
    Blitz->FinishPullTarget();
    TestTrue(TEXT("Timeout release also resolves caster overlap before restoring collision"),
        FVector::Dist2D(Blitz->GetActorLocation(), Target->GetActorLocation()) >= MinimumSeparation + 5.f);
    TestFalse(TEXT("Timeout releases movement lock"), Target->IsGrabbed());
    TestFalse(TEXT("Pull timer is cleared"), World->GetTimerManager().TimerExists(Blitz->PullTimerHandle));
    TestFalse(TEXT("Timeout timer is cleared"), World->GetTimerManager().TimerExists(Blitz->PullTimeoutTimerHandle));
    return true;
}

#endif
