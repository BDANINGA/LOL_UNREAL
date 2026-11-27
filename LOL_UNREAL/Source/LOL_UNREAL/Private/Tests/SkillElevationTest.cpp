#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/SkillCollision.h"
#include "BaseChampion.h"
#include "Champion/Champion_Ezreal.h"
#include "Champion/Projectile/PlanarSkillProjectileMovement.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillElevationTest, "LOL.Combat.Skills.PlanarCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillElevationTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto AddBody = [&](FVector Position)
    {
        auto* Body = World->SpawnActor<ABaseChampion>(Position, FRotator::ZeroRotator, Spawn);
        Body->GetCapsuleComponent()->SetCapsuleRadius(40.f);
        return Body;
    };
    auto* Low = AddBody(FVector(300, 0, -5000));
    auto* High = AddBody(FVector(700, 0, 5000));
    auto* Outside = AddBody(FVector(200, 200, 0));
    auto* Beyond = AddBody(FVector(1100, 0, 0));
    auto* Dead = AddBody(FVector(100, 0, 0));
    Dead->StateComponent->AddStatusTag(LOLTags::State_Dead);
    auto* Disabled = AddBody(FVector(150, 0, 0));
    Disabled->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // A large detection sensor must not make a body outside the skill path hittable.
    auto* Sensor = NewObject<USphereComponent>(Outside);
    Sensor->SetupAttachment(Outside->GetRootComponent());
    Sensor->InitSphereRadius(1000.f);
    Sensor->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Sensor->RegisterComponent();
    TArray<FHitResult> Hits;
    LOLSkillCollision::Sweep(World, Hits, FVector::ZeroVector, FVector(1000, 0, 0), 50.f);
    if (!TestEqual(TEXT("Only the two live bodies on the path are hit"), Hits.Num(), 2)) return false;
    TestTrue(TEXT("Low target hit first"), Hits[0].GetActor() == Low);
    TestTrue(TEXT("High target hit second"), Hits[1].GetActor() == High);
    TestEqual(TEXT("Contact uses combined body and skill radius"), Hits[0].Distance, 210.f);
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(Low);
    LOLSkillCollision::Sweep(World, Hits, FVector::ZeroVector, FVector(1000, 0, 0), 50.f, Query);
    TestEqual(TEXT("Ignored caster/body is excluded"), Hits.Num(), 1);
    LOLSkillCollision::Sweep(World, Hits, FVector(300, 0, 9000), FVector(300, 0, 9000), 50.f);
    TestTrue(TEXT("Stationary area query ignores height"), Hits.Num() == 1 && Hits[0].GetActor() == Low);
    Low->SetActorLocation(FVector(300, 90, -5000));
    LOLSkillCollision::Sweep(World, Hits, FVector::ZeroVector, FVector(400, 0, 0), 50.f);
    TestEqual(TEXT("A tangent body is hit"), Hits.Num(), 1);
    Low->SetActorLocation(FVector(300, 91, -5000));
    LOLSkillCollision::Sweep(World, Hits, FVector::ZeroVector, FVector(400, 0, 0), 50.f);
    TestTrue(TEXT("Moving just outside combined radii dodges"), Hits.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEzrealElevationTest, "LOL.Combat.Ezreal.Elevation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEzrealElevationTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    for (uint8 Type = 0; Type < 3; ++Type)
    for (float Height : {-2000.f, 2000.f})
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
        auto* Ezreal = World->SpawnActor<AChampion_Ezreal>(FVector(-200, 0, 0), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Ezreal"), Ezreal)) return false;
        Ezreal->StateComponent->AddStatusTag(LOLTags::Team_Blue);
        auto AddTarget = [&](FVector Position, bool Enemy = true)
        {
            auto* Target = World->SpawnActor<ABaseChampion>(Position, FRotator::ZeroRotator, Spawn);
            Target->StateComponent->AddStatusTag(Enemy ? LOLTags::Team_Red : LOLTags::Team_Blue);
            FChampionStat Stat;
            Stat.MaxHP = 10000.f;
            Target->StatComponent->SetStat(Stat);
            Target->StatComponent->SetHP(10000.f);
            return Target;
        };
        auto* Near = AddTarget(FVector(400, 0, Height));
        auto* Far = AddTarget(FVector(800, 0, 0));
        auto* Ally = AddTarget(FVector(200, 0, Height), false);
        auto* Dodge = AddTarget(FVector(300, 400, Height));
        auto* Beyond = AddTarget(FVector(1300, 0, Height));
        Ezreal->SpawnEzrealProjectileVisual(Type, FVector::ZeroVector, FVector(1000, 0, 0),
            1.f, 50.f, 100.f, ULOL_DamageTrueDamage::StaticClass(), Type == 2, 123);
        if (!TestEqual(TEXT("One active projectile"), Ezreal->ActiveProjectiles.Num(), 1)) return false;
        AActor* Shot = Ezreal->ActiveProjectiles.CreateIterator().Key();
        auto* Movement = Shot->FindComponentByClass<UPlanarSkillProjectileMovement>();
        if (!TestNotNull(TEXT("Planar flight component"), Movement)) return false;
        Movement->TickComponent(.1f, LEVELTICK_All, nullptr);
        TestEqual(TEXT("Flight has no early damage"), Near->StatComponent->GetCurrentHP(), 10000.f);
        Movement->TickComponent(.4f, LEVELTICK_All, nullptr);
        TestEqual(TEXT("Q/W/R hit across either elevation"), Near->StatComponent->GetCurrentHP(), 9900.f);
        if (Type == 2)
        {
            Movement->TickComponent(.02f, LEVELTICK_All, nullptr);
            TestEqual(TEXT("Piercing R cannot damage the same body twice"), Near->StatComponent->GetCurrentHP(), 9900.f);
            Movement->TickComponent(2.f, LEVELTICK_All, nullptr);
            TestEqual(TEXT("R pierces to the next target"), Far->StatComponent->GetCurrentHP(), 9900.f);
        }
        else
        {
            TestTrue(TEXT("Q/W stop at the first eligible enemy"), Shot->IsActorBeingDestroyed());
            TestEqual(TEXT("Enemy behind the first contact is unharmed"), Far->StatComponent->GetCurrentHP(), 10000.f);
        }
        TestEqual(TEXT("Ally is ignored"), Ally->StatComponent->GetCurrentHP(), 10000.f);
        TestEqual(TEXT("Body outside the path is ignored"), Dodge->StatComponent->GetCurrentHP(), 10000.f);
        TestEqual(TEXT("Long frame cannot extend range"), Beyond->StatComponent->GetCurrentHP(), 10000.f);
    }
    return true;
}
#endif
