#if WITH_DEV_AUTOMATION_TESTS

#include "Champion/Champion_LeeSin.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeeSinQElevationTest, "LOL.Combat.LeeSin.QElevation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeeSinQElevationTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    for (float Height : {-2000.f, 2000.f})
    for (int32 Scenario = 0; Scenario < 3; ++Scenario)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(TEXT("World"), World)) return false;
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
        World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        FURL URL;
        URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
        if (!TestTrue(TEXT("Authority game mode"), World->SetGameMode(URL))) return false;
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* LeeSin = World->SpawnActor<AChampion_LeeSin>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Lee Sin"), LeeSin)) return false;
        LeeSin->StateComponent->AddStatusTag(LOLTags::Team_Blue);
        FChampionStat Stat;
        Stat.Level = 1;
        Stat.MaxHP = 10000.f;
        LeeSin->StatComponent->SetStat(Stat);
        LeeSin->SkillComponent->Owner = LeeSin;
        LeeSin->SkillComponent->InitializeSkills();
        LeeSin->SkillComponent->Server_LevelUpSkill("Q");
        FSkillData& Q = LeeSin->SkillComponent->GetQ_Data();
        Q.ManaCost = {0.f};
        Q.Cooldown = {0.f};
        Q.Range = {1000.f};
        Q.BaseDamage = {100.f};
        LeeSin->QProjectileSpeed = 1000.f;

        auto AddTarget = [&](FVector Position, bool Enemy = true)
        {
            auto* Target = World->SpawnActor<ABaseChampion>(Position, FRotator::ZeroRotator, Spawn);
            Target->StateComponent->AddStatusTag(Enemy ? LOLTags::Team_Red : LOLTags::Team_Blue);
            Target->StatComponent->SetStat(Stat);
            Target->StatComponent->SetHP(10000.f);
            return Target;
        };
        auto* Near = AddTarget(FVector(Scenario == 2 ? 1400.f : 400.f, 0, Height));
        auto* Far = AddTarget(FVector(800, Scenario == 0 ? 0 : 400, 0));
        auto* Ally = AddTarget(FVector(200, 0, Height), false);
        auto* Dead = AddTarget(FVector(250, 0, Height));
        Dead->StateComponent->AddStatusTag(LOLTags::State_Dead);
        LeeSin->Server_Skill_Q_Implementation(FVector(1000, 0, Height));
        if (!TestTrue(TEXT("Q launches through the server cast path"), LeeSin->bQProjectileActive)) return false;
        LeeSin->UpdateQProjectile(.1f);
        TestFalse(TEXT("Q does not mark before arrival"), LeeSin->bQMarkActive);
        TestEqual(TEXT("Q does not damage before arrival"), Near->StatComponent->GetCurrentHP(), 10000.f);
        if (Scenario == 1) Near->SetActorLocation(FVector(400, 400, Height));
        LeeSin->UpdateQProjectile(2.f);
        TestFalse(TEXT("Projectile ends even on a long frame"), LeeSin->bQProjectileActive);
        TestEqual(TEXT("Allies do not block or receive Q"), Ally->StatComponent->GetCurrentHP(), 10000.f);
        TestEqual(TEXT("Enemy behind the first contact remains unharmed"), Far->StatComponent->GetCurrentHP(), 10000.f);
        if (Scenario != 0)
        {
            TestFalse(TEXT("Dodging or exceeding range leaves no mark"), LeeSin->bQMarkActive);
            TestEqual(TEXT("Dodging or exceeding range avoids damage"), Near->StatComponent->GetCurrentHP(), 10000.f);
            continue;
        }
        TestTrue(TEXT("First elevated enemy receives the mark"), LeeSin->bQMarkActive && LeeSin->QMarkedTarget == Near);
        const float AfterQ = Near->StatComponent->GetCurrentHP();
        TestTrue(TEXT("Elevated Q deals damage"), AfterQ < 10000.f);
        LeeSin->UpdateQProjectile(1.f);
        TestEqual(TEXT("Q cannot hit twice"), Near->StatComponent->GetCurrentHP(), AfterQ);

        Near->SetActorLocation(FVector(1400, 0, Height));
        LeeSin->Server_Skill_Q_Implementation(FVector::ZeroVector);
        TestFalse(TEXT("Q recast still respects horizontal range"), LeeSin->bIsQDashing);
        Near->SetActorLocation(FVector(400, 0, Height));
        Near->SetActorScale3D(FVector(1.5f));
        const auto CollisionBefore = LeeSin->GetCapsuleComponent()->GetCollisionEnabled();
        LeeSin->Server_Skill_Q_Implementation(FVector::ZeroVector);
        if (!TestTrue(TEXT("Q recast reaches targets on either elevation"), LeeSin->bIsQDashing)) return false;
        TestFalse(TEXT("Q recast consumes the mark"), LeeSin->bQMarkActive);
        LeeSin->UpdateQDash(LeeSin->QDashDuration);
        TestFalse(TEXT("Q dash completes"), LeeSin->bIsQDashing);
        const double LeeFeet = LeeSin->GetActorLocation().Z - LeeSin->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const double TargetFeet = Near->GetActorLocation().Z - Near->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        TestTrue(TEXT("Q lands at the target's floor height"), FMath::IsNearlyEqual(LeeFeet, TargetFeet, .1));
        TestTrue(TEXT("Q recast deals follow-up damage"), Near->StatComponent->GetCurrentHP() < AfterQ);
        TestEqual(TEXT("Q dash restores collision"), LeeSin->GetCapsuleComponent()->GetCollisionEnabled(), CollisionBefore);
    }
    return true;
}
#endif
