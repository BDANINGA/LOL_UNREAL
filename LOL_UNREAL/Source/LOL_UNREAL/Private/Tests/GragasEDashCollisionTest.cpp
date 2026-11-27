#if WITH_DEV_AUTOMATION_TESTS

#include "Champion/Champion_Gragas.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGragasEDashCollisionTest, "LOL.Combat.Gragas.EDashCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGragasEDashCollisionTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    // Long/short frames, formerly oversized side hit, both elevations, range, scaled body.
    for (int32 Scenario = 0; Scenario < 7; ++Scenario)
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
        auto* Gragas = World->SpawnActor<AChampion_Gragas>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Gragas"), Gragas)) return false;
        Gragas->StateComponent->AddStatusTag(LOLTags::Team_Blue);
        if (Scenario == 6) Gragas->SetActorScale3D(FVector(1.5f));
        FChampionStat Stat;
        Stat.Level = 1;
        Stat.MaxHP = 10000.f;
        Gragas->StatComponent->SetStat(Stat);
        Gragas->SkillComponent->Owner = Gragas;
        Gragas->SkillComponent->InitializeSkills();
        Gragas->SkillComponent->Server_LevelUpSkill("E");
        FSkillData& E = Gragas->SkillComponent->GetE_Data();
        E.ManaCost = {0.f};
        E.Cooldown = {0.f};
        E.Range = {600.f};
        E.BaseDamage = {100.f};

        auto AddTarget = [&](FVector Position, bool Enemy = true)
        {
            auto* Target = World->SpawnActor<ABaseChampion>(Position, FRotator::ZeroRotator, Spawn);
            Target->StateComponent->AddStatusTag(Enemy ? LOLTags::Team_Red : LOLTags::Team_Blue);
            Target->StatComponent->SetStat(Stat);
            Target->StatComponent->SetHP(10000.f);
            return Target;
        };
        const float Height = Scenario == 3 ? -2000.f : (Scenario == 4 ? 2000.f : 0.f);
        auto* Target = AddTarget(FVector(Scenario == 5 ? 800.f : 400.f, Scenario == 2 ? 180.f : 0.f, Height));
        auto* Splash = AddTarget(FVector(400, 260, Height));
        auto* OutsideSplash = AddTarget(FVector(400, 320, Height));
        auto* Ally = AddTarget(FVector(160, 0, 0), false);
        auto* Dead = AddTarget(FVector(200, 0, 0));
        Dead->StateComponent->AddStatusTag(LOLTags::State_Dead);
        const float PreviousFrequency = Gragas->GetNetUpdateFrequency();
        Gragas->Server_Skill_E_Implementation(FVector(600, 0, 0));
        if (!TestTrue(TEXT("Server cast starts E"), Gragas->bEDashing)) return false;
        TestTrue(TEXT("Dash blocks input"), Gragas->IsMoveInputBlocked());
        Gragas->UpdateEDash(.1f);
        TestEqual(TEXT("No hit before arriving at contact"), Target->StatComponent->GetCurrentHP(), 10000.f);
        if (Scenario == 1)
        {
            for (int32 Step = 0; Step < 30 && Gragas->bEDashing; ++Step) Gragas->UpdateEDash(.01f);
        }
        else Gragas->UpdateEDash(1.f);
        TestFalse(TEXT("Dash completes"), Gragas->bEDashing);
        TestEqual(TEXT("Update frequency restored"), Gragas->GetNetUpdateFrequency(), PreviousFrequency);
        TestEqual(TEXT("Ally is ignored"), Ally->StatComponent->GetCurrentHP(), 10000.f);
        TestEqual(TEXT("Outside splash is unharmed"), OutsideSplash->StatComponent->GetCurrentHP(), 10000.f);
        if (Scenario == 2 || Scenario == 5)
        {
            TestEqual(TEXT("A body outside the swept capsule or range is missed"), Target->StatComponent->GetCurrentHP(), 10000.f);
            TestFalse(TEXT("A miss does not stun"), Target->bIsStunned);
            TestEqual(TEXT("A miss does not trigger splash damage"), Splash->StatComponent->GetCurrentHP(), 10000.f);
            TestTrue(TEXT("Miss ends exactly at range"), Gragas->GetActorLocation().Equals(FVector(600, 0, 0), .01));
        }
        else
        {
            const double ContactX = 400.0 - Gragas->GetCapsuleComponent()->GetScaledCapsuleRadius()
                - Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
            TestTrue(TEXT("Caster stops at actual swept contact, independent of frame length or elevation"),
                Gragas->GetActorLocation().Equals(FVector(ContactX, 0, 0), .01));
            TestTrue(TEXT("Direct hit damages and stuns"), Target->StatComponent->GetCurrentHP() < 10000.f && Target->bIsStunned);
            TestTrue(TEXT("Existing secondary damage and stun are retained"), Splash->StatComponent->GetCurrentHP() < 10000.f && Splash->bIsStunned);
        }
        const float AfterHit = Target->StatComponent->GetCurrentHP();
        Gragas->UpdateEDash(1.f);
        TestEqual(TEXT("Completed dash cannot hit again"), Target->StatComponent->GetCurrentHP(), AfterHit);
        TestEqual(TEXT("Movement is restored"), Gragas->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);

        // Exercise the receiving endpoint used by owning clients and observers.
        auto* Receiver = World->SpawnActor<AChampion_Gragas>(FVector(-1000, 0, 0), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Endpoint receiver"), Receiver)) return false;
        Receiver->ExchangeNetRoles(true);
        Receiver->AddMovementInput(FVector::ForwardVector, 1.f, true);
        Receiver->Multicast_SetEDashState_Implementation(true, FVector::ZeroVector);
        TestTrue(TEXT("Received dash state locks client input"), Receiver->IsMoveInputBlocked());
        TestTrue(TEXT("Received dash clears stale input"), Receiver->GetPendingMovementInputVector().IsNearlyZero());
        Receiver->UpdateEDash(1.f);
        TestTrue(TEXT("Client does not perform authoritative dash movement"), Receiver->GetActorLocation().IsNearlyZero());
        Receiver->Multicast_SetEDashState_Implementation(false, Gragas->GetActorLocation());
        TestFalse(TEXT("Received endpoint unlocks input"), Receiver->bEDashing);
        TestTrue(TEXT("Received endpoint matches server contact"), Receiver->GetActorLocation().Equals(Gragas->GetActorLocation()));
        TestEqual(TEXT("Received endpoint restores movement"), Receiver->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Walking);
        Receiver->ExchangeNetRoles(true);
    }
    return true;
}
#endif
