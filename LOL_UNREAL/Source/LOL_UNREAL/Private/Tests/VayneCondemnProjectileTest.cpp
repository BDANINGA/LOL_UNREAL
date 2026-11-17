#if WITH_DEV_AUTOMATION_TESTS
#include "Champion/Champion_Vayne.h"
#include "Champion/Projectile/VayneCondemnProjectile.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_FixedKnockbackComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVayneCondemnProjectileTest,
    "LOL.Combat.Vayne.CondemnProjectile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVayneCondemnProjectileTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    // APawn rejects damage without an authority game mode, even in a test world.
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    if (!TestTrue(TEXT("Authority game mode"), World->SetGameMode(URL))) return false;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Shooter = World->SpawnActor<AChampion_Vayne>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
    auto* Target = World->SpawnActor<ABaseChampion>(FVector(440,0,100), FRotator::ZeroRotator, Params);
    if (!Shooter || !Target) return false;
    Shooter->StateComponent->AddStatusTag(LOLTags::Team_Blue);
    Target->StateComponent->AddStatusTag(LOLTags::Team_Red);
    FChampionStat Stats;
    Stats.MaxHP = 10000.f;
    Stats.AttackDamage = 100.f;
    Shooter->StatComponent->SetStat(Stats);
    Target->StatComponent->SetStat(Stats);
    Target->StatComponent->SetHP(10000.f);

    auto SpawnBolt = [&]()
    {
        auto* Bolt = World->SpawnActor<AVayneCondemnProjectile>(FVector(0,0,100), FRotator::ZeroRotator, Params);
        Bolt->Initialize(Shooter, Target);
        return Bolt;
    };
    auto* Bolt = SpawnBolt();
    World->Tick(LEVELTICK_TimeOnly, .08f);
    Bolt->Tick(.08f);
    TestTrue(TEXT("Bolt travels before applying the hit"), Bolt->GetActorLocation().X > 0 && Bolt->GetActorLocation().X < 440);
    TestFalse(TEXT("No early knockback"), Target->bIsKnockedBack);
    TestEqual(TEXT("No early damage"), Target->StatComponent->GetCurrentHP(), 10000.f);
    Target->SetActorLocation(FVector(440,120,100));
    World->Tick(LEVELTICK_TimeOnly, .2f);
    Bolt->Tick(.2f);
    TestTrue(TEXT("Targeted bolt follows the target's movement"), Bolt->GetActorLocation().Equals(Target->GetActorLocation(), .01));
    TestTrue(TEXT("Arrival starts knockback"), Target->bIsKnockedBack);
    const float HealthAfterHit = Target->StatComponent->GetCurrentHP();
    TestTrue(TEXT("Arrival applies damage"), HealthAfterHit < 10000.f);
    Bolt->Tick(.1f);
    TestEqual(TEXT("Impact cannot apply damage twice"), Target->StatComponent->GetCurrentHP(), HealthAfterHit);
    if (auto* Knockback = Target->FindComponentByClass<ULOL_FixedKnockbackComponent>())
        Knockback->TickComponent(.4f, LEVELTICK_All, nullptr);

    auto* Cancelled = SpawnBolt();
    Target->StateComponent->AddStatusTag(LOLTags::State_Dead);
    Cancelled->Tick(.1f);
    TestTrue(TEXT("A dead target cancels the bolt"), Cancelled->IsActorBeingDestroyed());
    TestEqual(TEXT("Cancelled bolt does not deal damage"), Target->StatComponent->GetCurrentHP(), HealthAfterHit);
    return true;
}
#endif
