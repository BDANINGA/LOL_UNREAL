#if WITH_DEV_AUTOMATION_TESTS

#include "BaseChampion.h"
#include "Widget/LOL_HUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Component/LOL_StateComponent.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineGlobals.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecallBarTest, "LOL.HUD.RecallBar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRecallBarTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> ScriptExecutionGuard(GAllowActorScriptExecutionInEditor, true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->SetGameState(World->SpawnActor<AGameStateBase>());
    APlayerController* Player = World->SpawnActor<APlayerController>();
    Player->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    World->AddController(Player);
    ABaseChampion* Champion = World->SpawnActor<ABaseChampion>();
    Player->Possess(Champion);

    UClass* HUDClass = LoadClass<ULOL_HUDWidget>(nullptr, TEXT("/Game/UI/HUD/Wbp_lol_hud.Wbp_lol_hud_C"));
    if (!TestNotNull(TEXT("Existing HUD blueprint"), HUDClass)) return false;
    ULOL_HUDWidget* HUD = CreateWidget<ULOL_HUDWidget>(Player, HUDClass);
    if (!TestNotNull(TEXT("HUD instance"), HUD)) return false;
    if (!TestTrue(TEXT("HUD resolves the possessed champion"), HUD->GetOwningPlayerPawn() == Champion)) return false;
    const auto SlateWidget = HUD->TakeWidget();
    UBorder* Container = Cast<UBorder>(HUD->WidgetTree->FindWidget(TEXT("RuntimeRecallBar")));
    UProgressBar* Bar = Cast<UProgressBar>(HUD->WidgetTree->FindWidget(TEXT("RecallProgressBar")));
    UTextBlock* Time = Cast<UTextBlock>(HUD->WidgetTree->FindWidget(TEXT("RecallTimeText")));
    if (!TestNotNull(TEXT("Recall container"), Container) || !TestNotNull(TEXT("Recall bar"), Bar) ||
        !TestNotNull(TEXT("Remaining time label"), Time)) return false;

    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    auto Advance = [World, HUD](float Seconds)
    {
        while (Seconds > KINDA_SMALL_NUMBER)
        {
            const float Step = FMath::Min(Seconds, 0.1f);
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, Step);
            Seconds -= Step;
        }
        HUD->UpdateRecallBar();
    };
    Advance(0.01f);
    TestTrue(TEXT("Idle bar is hidden"), Container->GetVisibility() == ESlateVisibility::Collapsed);
    Champion->StartRecall();
    HUD->UpdateRecallBar();
    TestEqual(TEXT("Recall still lasts eight seconds"), Champion->GetRecallTimeRemaining(), 8.0f);
    TestEqual(TEXT("Bar starts full"), Bar->GetPercent(), 1.0f);
    TestTrue(TEXT("Recall bar is visible without intercepting clicks"), Container->GetVisibility() == ESlateVisibility::HitTestInvisible);
    TestFalse(TEXT("Time is displayed"), Time->GetText().IsEmpty());
    Advance(4.0f);
    TestTrue(TEXT("Bar is halfway after four seconds"), FMath::IsNearlyEqual(Bar->GetPercent(), 0.5f, 0.01f));
    TestTrue(TEXT("Four seconds remain"), FMath::IsNearlyEqual(Champion->GetRecallTimeRemaining(), 4.0f, 0.01f));

    // Optional real UMG render for layout verification (run without -NullRHI).
    if (FParse::Param(FCommandLine::Get(), TEXT("RecallBarScreenshot")))
    {
        FWidgetRenderer Renderer(true);
        UTextureRenderTarget2D* Target = Renderer.DrawWidget(SlateWidget, FVector2D(1920.0f, 1080.0f));
        FImage Screenshot;
        if (TestTrue(TEXT("HUD screenshot renders"), FImageUtils::GetRenderTargetImage(Target, Screenshot)))
        {
            const FString Directory = FPaths::ProjectSavedDir() / TEXT("Validation/RecallBar");
            IFileManager::Get().MakeDirectory(*Directory, true);
            TestTrue(TEXT("HUD screenshot saved"), FImageUtils::SaveImageByExtension(*(Directory / TEXT("RecallBar.png")), Screenshot));
        }
    }

    Champion->StartRecall();
    TestTrue(TEXT("Repeated recall input does not restart the timer"), FMath::IsNearlyEqual(Champion->GetRecallTimeRemaining(), 4.0f, 0.01f));
    Champion->CancelRecall();
    HUD->UpdateRecallBar();
    TestTrue(TEXT("Cancelled bar is hidden"), Container->GetVisibility() == ESlateVisibility::Collapsed);
    TestEqual(TEXT("Cancellation resets progress"), Bar->GetPercent(), 0.0f);
    TestTrue(TEXT("Cancellation clears time text"), Time->GetText().IsEmpty());

    Champion->StartRecall();
    Advance(7.9f);
    TestTrue(TEXT("Bar is nearly empty before completion"), Bar->GetPercent() > 0.0f && Bar->GetPercent() < 0.02f);
    TestTrue(TEXT("Channel does not finish early"), Champion->IsRecallActive());
    Advance(0.2f);
    TestFalse(TEXT("Server timer completes recall"), Champion->IsRecallActive());
    TestTrue(TEXT("Completed bar is hidden"), Container->GetVisibility() == ESlateVisibility::Collapsed);
    TestEqual(TEXT("Completion clears remaining time"), Champion->GetRecallTimeRemaining(), 0.0f);

    Champion->StartRecall();
    Champion->StateComponent->AddStatusTag(LOLTags::State_Dead);
    HUD->UpdateRecallBar();
    TestTrue(TEXT("Death hides the bar"), Container->GetVisibility() == ESlateVisibility::Collapsed);
    Champion->CancelRecall();
    Player->UnPossess();
    HUD->UpdateRecallBar();
    TestTrue(TEXT("Missing pawn keeps the bar hidden"), Container->GetVisibility() == ESlateVisibility::Collapsed);
    return true;
}

#endif
