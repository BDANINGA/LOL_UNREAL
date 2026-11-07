#if WITH_DEV_AUTOMATION_TESTS

#include "BaseChampion.h"
#include "Champion/Champion_Alistar.h"
#include "Champion/Champion_Ezreal.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockupCollisionTest,
    "LOL.Combat.Alistar.KnockupCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKnockupCollisionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    ABaseChampion* Champion = World->SpawnActor<AChampion_Ezreal>();
    if (!TestNotNull(TEXT("Champion"), Champion)) return false;
    ABaseChampion* OtherChampion = World->SpawnActor<AChampion_Alistar>(
        FVector(-400.f, 0.f, 0.f), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Other champion"), OtherChampion)) return false;

    UCapsuleComponent* Capsule = Champion->GetCapsuleComponent();
    UCharacterMovementComponent* Movement = Champion->GetCharacterMovement();
    const ECollisionResponse WorldResponse = Capsule->GetCollisionResponseToChannel(ECC_WorldStatic);
    const ECollisionResponse ProjectileResponse = Capsule->GetCollisionResponseToChannel(ECC_WorldDynamic);

    Movement->SetMovementMode(MOVE_Falling);
    Champion->SuppressChampionCollisionUntilLanding();
    TestEqual(TEXT("Airborne champions do not block each other"),
        Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2), ECR_Overlap);
    TestEqual(TEXT("Terrain collision remains enabled"),
        Capsule->GetCollisionResponseToChannel(ECC_WorldStatic), WorldResponse);
    TestEqual(TEXT("Projectile collision remains enabled"),
        Capsule->GetCollisionResponseToChannel(ECC_WorldDynamic), ProjectileResponse);
    FHitResult AirborneHit;
    OtherChampion->SetActorLocation(FVector(400.f, 0.f, 0.f), true, &AirborneHit);
    TestFalse(TEXT("Another champion can move through the airborne capsule without pushing it"),
        AirborneHit.bBlockingHit);

    // The old Q timer clears this flag before some knockups have landed.
    Champion->SetIsKnockedBack(false);
    TestEqual(TEXT("Knockback timer does not restore airborne blocking"),
        Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2), ECR_Overlap);
    Champion->SuppressChampionCollisionUntilLanding();
    Movement->SetMovementMode(MOVE_Walking);
    TestEqual(TEXT("Landing restores original blocking after repeated knockup"),
        Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2), ECR_Block);
    OtherChampion->SetActorLocation(FVector(-400.f, 0.f, 0.f));
    FHitResult GroundedHit;
    OtherChampion->SetActorLocation(FVector(400.f, 0.f, 0.f), true, &GroundedHit);
    TestTrue(TEXT("Champions block each other again after landing"), GroundedHit.bBlockingHit);

    Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
    Movement->SetMovementMode(MOVE_Falling);
    Champion->SuppressChampionCollisionUntilLanding();
    Movement->SetMovementMode(MOVE_None);
    TestEqual(TEXT("Interrupted falling restores the saved response"),
        Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2), ECR_Ignore);
    return true;
}

#endif
