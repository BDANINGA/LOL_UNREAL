#if WITH_DEV_AUTOMATION_TESTS

#include "BaseChampion.h"
#include "Building/BaseBuilding.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
#include "Component/LOL_StatComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageTextTest, "LOL.Combat.DamageText.OutgoingTargets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDamageTextTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APlayerController* Player = World->SpawnActor<APlayerController>();
    ABaseChampion* Attacker = World->SpawnActor<ABaseChampion>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (!Player || !Attacker) return false;
    Player->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Player->Possess(Attacker);
    TestTrue(TEXT("Local player owns the attacker"), Attacker->GetController() == Player && Player->IsLocalController());
    UClass* FloaterClass = Attacker->FindComponentByClass<ULOL_StatComponent>()->DamageFloaterClass;
    if (!TestNotNull(TEXT("Cooked floater class reference"), FloaterClass)) return false;

    auto Floaters = [&]()
    {
        TArray<AActor*> Result;
        for (TActorIterator<AActor> It(World, FloaterClass); It; ++It) Result.Add(*It);
        return Result;
    };
    auto CheckText = [&](AActor* Floater, const TCHAR* Amount, FColor Color)
    {
        UWidgetComponent* Component = Floater->FindComponentByClass<UWidgetComponent>();
        UUserWidget* Widget = Component ? Component->GetUserWidgetObject() : nullptr;
        UTextBlock* Text = Widget && Widget->WidgetTree
            ? Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("text_damage"))) : nullptr;
        if (!TestNotNull(TEXT("Real damage widget text"), Text)) return;
        TestEqual(TEXT("Actual damage amount"), Text->GetText().ToString(), FString(Amount));
        TestTrue(TEXT("Target category color"), Text->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::FromSRGBColor(Color)));
        TestNotNull(TEXT("Font available"), Text->GetFont().FontObject.Get());
        TestFalse(TEXT("Cosmetic floater stays local"), Floater->GetIsReplicated());
        TestTrue(TEXT("Floater expires"), Floater->GetLifeSpan() > 0.f);
    };

    UClass* Targets[] = {ABaseMinion::StaticClass(), ABaseBuilding::StaticClass(),
        ABaseJungleMonster::StaticClass(), ABaseChampion::StaticClass()};
    for (UClass* TargetClass : Targets)
    {
        AActor* Victim = World->SpawnActor<AActor>(TargetClass, FVector(500, 0, 0), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Damage target"), Victim)) return false;
        ULOL_StatComponent* Stats = Victim->FindComponentByClass<ULOL_StatComponent>();
        FChampionStat Data; Data.MaxHP = 100.f;
        Stats->SetStat(Data); Stats->SetHP(100.f); Stats->bEnableHitImpact = false;
        const FColor Color = Victim->IsA<ABaseChampion>() ? FColor(255, 128, 48) : FColor(255, 224, 80);
        // Exercise the real accepted-damage path, not the presentation RPC alone.
        TestEqual(TEXT("Outgoing hit accepted"), Stats->ApplyDamage(37.f, EDamageType::TrueDamage, Player, Attacker), 37.f);
        TArray<AActor*> Spawned = Floaters();
        if (!TestEqual(TEXT("Exactly one floater per outgoing hit"), Spawned.Num(), 1)) return false;
        CheckText(Spawned[0], TEXT("37"), Color);
        TestTrue(TEXT("Text above target body"), Spawned[0]->GetActorLocation().Z > Victim->GetActorLocation().Z);
        Spawned[0]->Destroy();
        Stats->ApplyDamage(0.f, EDamageType::TrueDamage, Player, Attacker);
        TestEqual(TEXT("Zero damage suppressed"), Floaters().Num(), 0);
        Stats->ApplyDamage(1.f, EDamageType::TrueDamage, nullptr, nullptr);
        TestEqual(TEXT("Unrelated combat suppressed"), Floaters().Num(), 0);
        // Missing controller falls back to damage causer; lethal damage shows remaining HP.
        Stats->ApplyDamage(1000.f, EDamageType::TrueDamage, nullptr, Attacker);
        Spawned = Floaters();
        if (!TestEqual(TEXT("Lethal outgoing hit shown"), Spawned.Num(), 1)) return false;
        CheckText(Spawned[0], TEXT("62"), Color);
        Spawned[0]->Destroy();
        Victim->Destroy();
    }
    ULOL_StatComponent* OwnStats = Attacker->FindComponentByClass<ULOL_StatComponent>();
    FChampionStat Data; Data.MaxHP = 100.f;
    OwnStats->SetStat(Data); OwnStats->SetHP(100.f); OwnStats->bEnableHitImpact = false;
    OwnStats->ApplyDamage(12.f, EDamageType::TrueDamage, nullptr, nullptr);
    const TArray<AActor*> Incoming = Floaters();
    if (TestEqual(TEXT("Incoming damage still shown"), Incoming.Num(), 1))
        CheckText(Incoming[0], TEXT("12"), FColor(255, 128, 48));
    return true;
}

#endif
