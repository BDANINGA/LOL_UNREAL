// HUD 관리하는 위젯 클래스
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Components/Widget.h"

#include "Component/Champion_SkillComponent.h"

#include "LOL_GameState.h"
#include "LOL_PlayerState.h"
#include "LOL_PlayerController.h"
#include "BaseChampion.h"
#include "Widget/LOL_HUDWidget.h"

#include "Kismet/GameplayStatics.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "GameFramework/PlayerController.h"
#include "PaperSprite.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ULOL_HUDWidget::ULOL_HUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UUserWidget> KillLogWidgetFinder(
        TEXT("/Game/UI/wbp_kill_log.wbp_kill_log_C"));
    if (KillLogWidgetFinder.Succeeded())
    {
        KillLogWidgetClass = KillLogWidgetFinder.Class;
    }

    static ConstructorHelpers::FObjectFinder<UPaperSprite> EXPProgressFillSpriteFinder(
        TEXT("/Script/Paper2D.PaperSprite'/Game/UI/HUD/tex/clarity_hudatlasupdate_Sprite_278.clarity_hudatlasupdate_Sprite_278'"));
    if (EXPProgressFillSpriteFinder.Succeeded())
    {
        EXPProgressFillSprite = EXPProgressFillSpriteFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> ExhaustIconFinder(
        TEXT("/Game/LOL_Data/Data_SummonerSpell/summoners_spell_exaust.summoners_spell_exaust"));
    if (ExhaustIconFinder.Succeeded())
    {
        ExhaustIcon = ExhaustIconFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> FlashIconFinder(
        TEXT("/Game/LOL_Data/Data_SummonerSpell/summoners_spell_flash.summoners_spell_flash"));
    if (FlashIconFinder.Succeeded())
    {
        FlashIcon = FlashIconFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> CooldownMaterialFinder(
        TEXT("/Game/UI/HUD/M_skillcooldown.M_skillcooldown"));
    if (CooldownMaterialFinder.Succeeded())
    {
        SkillCooldownMaterial = CooldownMaterialFinder.Object;
    }
}

void ULOL_HUDWidget::UpdateSkillDisableOverlays()
{
    const ABaseChampion* Champion = Cast<ABaseChampion>(GetOwningPlayerPawn());
    const UChampion_SkillComponent* SkillComp = Champion ? Champion->SkillComponent : nullptr;

    if (!SkillComp)
    {
        return;
    }

    // 각 스킬의 찍힌 레벨 확인 (0레벨이면 안 찍힌 상태)
    auto SetOverlayVisibility = [SkillComp](UImage* DisableImage, FName SkillName)
        {
            if (!DisableImage)
            {
                return;
            }

            // 스킬을 찍었는지(CanLevelUp 조건 또는 스킬 레벨 체크)
            // 만약 SkillComponent에 GetSkillLevel(SkillName) 함수가 있다면 사용:
            const int32 SkillLevel = SkillComp->GetSkillLevel(SkillName);

            if (SkillLevel > 0)
            {
                // 스킬을 찍었으면 어두운 이미지 숨김 (밝아짐)
                DisableImage->SetVisibility(ESlateVisibility::Collapsed); // 또는 Hidden
            }
            else
            {
                // 스킬을 안 찍었으면 어두운 이미지 표시
                DisableImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // 또는 Visible
            }
        };

    SetOverlayVisibility(SkillQ_DisableImage, TEXT("Q"));
    SetOverlayVisibility(SkillW_DisableImage, TEXT("W"));
    SetOverlayVisibility(SkillE_DisableImage, TEXT("E"));
    SetOverlayVisibility(SkillR_DisableImage, TEXT("R"));
}

void ULOL_HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SkillQ_Image) SkillQ_MID = SkillQ_Image->GetDynamicMaterial();
    if (SkillW_Image) SkillW_MID = SkillW_Image->GetDynamicMaterial();
    if (SkillE_Image) SkillE_MID = SkillE_Image->GetDynamicMaterial();
    if (SkillR_Image) SkillR_MID = SkillR_Image->GetDynamicMaterial();

    CacheSummonerSpellWidgets();
    CacheMinimapChampionImages();

    CacheItemSlotImages();
    CacheScoreboardTextBlocks();
    CacheSkillLevelUpButtons();
    CacheEXPWidgets();
    CreateKillLogContainer();

    if (EXPProgressBar)
    {
        ApplyEXPProgressBarStyle();
    }
    ApplyEXPPercent(0.0f);

    SetLevel(1);

    if (ALOL_GameState* GameState = GetWorld()
        ? GetWorld()->GetGameState<ALOL_GameState>()
        : nullptr)
    {
        GameState->OnChampionKill.RemoveAll(this);
        GameState->OnChampionKill.AddUObject(this, &ULOL_HUDWidget::HandleChampionKill);
    }
}

void ULOL_HUDWidget::NativeDestruct()
{
    if (ALOL_GameState* GameState = GetWorld()
        ? GetWorld()->GetGameState<ALOL_GameState>()
        : nullptr)
    {
        GameState->OnChampionKill.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void ULOL_HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ApplyEXPPercent(CurrentEXPPercent);

    if (SkillQ_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeQ - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeQ, 0.0f, 1.0f);
        }

        SkillQ_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillW_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeW - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeW, 0.0f, 1.0f);
        }

        SkillW_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillE_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeE - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeE, 0.0f, 1.0f);
        }

        SkillE_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SkillR_MID)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();

        float RemainingTime = SkillCoolLocalEndTimeR - CurrentTime;

        float CooldownPercent = 0.0f;
        if (RemainingTime > 0.0f)
        {
            CooldownPercent = FMath::Clamp(RemainingTime / SkillCoolEndTimeR, 0.0f, 1.0f);
        }

        SkillR_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (SummonerSpell1_MID)
    {
        const float RemainingTime = SummonerSpell1CoolLocalEndTime - CurrentTime;
        const float CooldownPercent = RemainingTime > 0.0f && SummonerSpell1CoolEndTime > 0.0f
            ? FMath::Clamp(RemainingTime / SummonerSpell1CoolEndTime, 0.0f, 1.0f)
            : 0.0f;
        SummonerSpell1_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }
    if (SummonerSpell2_MID)
    {
        const float RemainingTime = SummonerSpell2CoolLocalEndTime - CurrentTime;
        const float CooldownPercent = RemainingTime > 0.0f && SummonerSpell2CoolEndTime > 0.0f
            ? FMath::Clamp(RemainingTime / SummonerSpell2CoolEndTime, 0.0f, 1.0f)
            : 0.0f;
        SummonerSpell2_MID->SetScalarParameterValue(TEXT("Cooldown"), CooldownPercent);
    }

    UpdateScoreboard();
    UpdateSkillLevelUpButtonStates();
    UpdateSkillDisableOverlays();
    UpdateMinimapChampions();
}

void ULOL_HUDWidget::UpdateScoreboard()
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (const ALOL_GameState* GameState = World->GetGameState<ALOL_GameState>())
    {
        if (BlueKillCountText)
        {
            BlueKillCountText->SetText(FText::AsNumber(GameState->BlueTeamKills));
        }
        if (RedKillCountText)
        {
            RedKillCountText->SetText(FText::AsNumber(GameState->RedTeamKills));
        }
        if (MatchTimerText)
        {
            const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(GameState->CurrentMatchTime));
            const int32 Minutes = TotalSeconds / 60;
            const int32 Seconds = TotalSeconds % 60;
            MatchTimerText->SetText(FText::FromString(
                FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
        }
    }

    if (const ABaseChampion* Champion = Cast<ABaseChampion>(GetOwningPlayerPawn()))
    {
        if (KdaCountText)
        {
            KdaCountText->SetText(FText::FromString(FString::Printf(
                TEXT("%02d/%02d/%02d"),
                Champion->KillCount,
                Champion->DeathCount,
                Champion->AssistCount)));
        }
        if (KillCountText)
        {
            KillCountText->SetText(FText::AsNumber(Champion->KillCount));
        }
        if (DeathCountText)
        {
            DeathCountText->SetText(FText::AsNumber(Champion->DeathCount));
        }
        if (AssistCountText)
        {
            AssistCountText->SetText(FText::AsNumber(Champion->AssistCount));
        }
        if (MinionCountText)
        {
            MinionCountText->SetText(FText::AsNumber(Champion->MinionKillCount));
        }
        return;
    }

    const APlayerController* PlayerController = GetOwningPlayer();
    const ALOL_PlayerState* PlayerState = PlayerController
        ? PlayerController->GetPlayerState<ALOL_PlayerState>()
        : nullptr;
    if (!PlayerState)
    {
        return;
    }

    if (KdaCountText)
    {
        KdaCountText->SetText(FText::FromString(FString::Printf(
            TEXT("%02d/%02d/%02d"),
            PlayerState->Kills,
            PlayerState->Deaths,
            PlayerState->Assists)));
    }
    if (KillCountText)
    {
        KillCountText->SetText(FText::AsNumber(PlayerState->Kills));
    }
    if (DeathCountText)
    {
        DeathCountText->SetText(FText::AsNumber(PlayerState->Deaths));
    }
    if (AssistCountText)
    {
        AssistCountText->SetText(FText::AsNumber(PlayerState->Assists));
    }
    if (MinionCountText)
    {
        MinionCountText->SetText(FText::AsNumber(PlayerState->MinionKills));
    }
}

void ULOL_HUDWidget::CreateKillLogContainer()
{
    if (KillLogContainer || !WidgetTree)
    {
        return;
    }

    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        return;
    }

    KillLogContainer = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),
        TEXT("RuntimeKillLogContainer"));
    if (!KillLogContainer)
    {
        return;
    }

    UCanvasPanelSlot* ContainerSlot = RootCanvas->AddChildToCanvas(KillLogContainer);
    ContainerSlot->SetAnchors(FAnchors(1.0f, 0.0f));
    ContainerSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    ContainerSlot->SetPosition(FVector2D(-40.0f, 500.0f));
    ContainerSlot->SetAutoSize(true);
    ContainerSlot->SetZOrder(100);
}

void ULOL_HUDWidget::HandleChampionKill(ABaseChampion* Killer, ABaseChampion* Victim)
{
    if (!Killer || !Victim || !KillLogWidgetClass)
    {
        return;
    }

    if (!KillLogContainer)
    {
        CreateKillLogContainer();
    }
    if (!KillLogContainer)
    {
        return;
    }

    UUserWidget* Entry = CreateWidget<UUserWidget>(GetOwningPlayer(), KillLogWidgetClass);
    if (!Entry || !Entry->WidgetTree)
    {
        return;
    }

    UImage* KillerPortrait = Cast<UImage>(Entry->WidgetTree->FindWidget(TEXT("kill_champ")));
    UImage* VictimPortrait = Cast<UImage>(Entry->WidgetTree->FindWidget(TEXT("death_champ")));

    if (KillerPortrait && Killer->ChampionResource.Portrait)
    {
        KillerPortrait->SetBrushFromTexture(Killer->ChampionResource.Portrait, false);
    }
    if (VictimPortrait && Victim->ChampionResource.Portrait)
    {
        VictimPortrait->SetBrushFromTexture(Victim->ChampionResource.Portrait, false);
    }

    while (KillLogContainer->GetChildrenCount() >= MaxKillLogEntries)
    {
        KillLogContainer->RemoveChildAt(0);
    }

    KillLogContainer->AddChildToVerticalBox(Entry);

    TWeakObjectPtr<UUserWidget> WeakEntry = Entry;
    FTimerHandle RemovalTimer;
    GetWorld()->GetTimerManager().SetTimer(
        RemovalTimer,
        FTimerDelegate::CreateWeakLambda(this, [this, WeakEntry]()
        {
            RemoveKillLogEntry(WeakEntry.Get());
        }),
        KillLogDisplayDuration,
        false);
}

void ULOL_HUDWidget::RemoveKillLogEntry(UUserWidget* Entry)
{
    if (Entry)
    {
        Entry->RemoveFromParent();
    }
}

void ULOL_HUDWidget::CacheSkillLevelUpButtons()
{
    if (!WidgetTree)
    {
        return;
    }

    auto FindLevelUpWidget = [this](UButton* BoundButton,
        std::initializer_list<const TCHAR*> CandidateNames)
    {
        if (BoundButton)
        {
            return static_cast<UWidget*>(BoundButton);
        }

        for (const TCHAR* CandidateName : CandidateNames)
        {
            if (UWidget* Widget = WidgetTree->FindWidget(FName(CandidateName)))
            {
                return Widget;
            }
        }

        return static_cast<UWidget*>(nullptr);
    };

    QLevelUpWidget = FindLevelUpWidget(QLevelUpButton, {
        TEXT("QLevelUpButton"),
        TEXT("Q_LevelUpButton"),
        TEXT("Button_Q_LevelUp"),
        TEXT("QPlusButton"),
        TEXT("SkillQLevelUpButton"),
        TEXT("SkillQ_LevelUpButton"),
        TEXT("LevelUp_Q")
    });
    WLevelUpWidget = FindLevelUpWidget(WLevelUpButton, {
        TEXT("WLevelUpButton"),
        TEXT("W_LevelUpButton"),
        TEXT("Button_W_LevelUp"),
        TEXT("WPlusButton"),
        TEXT("SkillWLevelUpButton"),
        TEXT("SkillW_LevelUpButton"),
        TEXT("LevelUp_W")
    });
    ELevelUpWidget = FindLevelUpWidget(ELevelUpButton, {
        TEXT("ELevelUpButton"),
        TEXT("E_LevelUpButton"),
        TEXT("Button_E_LevelUp"),
        TEXT("EPlusButton"),
        TEXT("SkillELevelUpButton"),
        TEXT("SkillE_LevelUpButton"),
        TEXT("LevelUp_E")
    });
    RLevelUpWidget = FindLevelUpWidget(RLevelUpButton, {
        TEXT("RLevelUpButton"),
        TEXT("R_LevelUpButton"),
        TEXT("Button_R_LevelUp"),
        TEXT("RPlusButton"),
        TEXT("SkillRLevelUpButton"),
        TEXT("SkillR_LevelUpButton"),
        TEXT("LevelUp_R")
    });

    if (!QLevelUpWidget && !WLevelUpWidget && !ELevelUpWidget && !RLevelUpWidget)
    {
        QLevelUpWidget = WidgetTree->FindWidget(TEXT("Button_0"));
        WLevelUpWidget = WidgetTree->FindWidget(TEXT("Button_1"));
        ELevelUpWidget = WidgetTree->FindWidget(TEXT("Button_2"));
        RLevelUpWidget = WidgetTree->FindWidget(TEXT("Button_3"));
    }

    QLevelUpButton = Cast<UButton>(QLevelUpWidget);
    WLevelUpButton = Cast<UButton>(WLevelUpWidget);
    ELevelUpButton = Cast<UButton>(ELevelUpWidget);
    RLevelUpButton = Cast<UButton>(RLevelUpWidget);

    if (QLevelUpButton)
    {
        QLevelUpButton->OnClicked.RemoveAll(this);
        QLevelUpButton->OnClicked.AddDynamic(this, &ULOL_HUDWidget::OnQLevelUpClicked);
    }
    if (WLevelUpButton)
    {
        WLevelUpButton->OnClicked.RemoveAll(this);
        WLevelUpButton->OnClicked.AddDynamic(this, &ULOL_HUDWidget::OnWLevelUpClicked);
    }
    if (ELevelUpButton)
    {
        ELevelUpButton->OnClicked.RemoveAll(this);
        ELevelUpButton->OnClicked.AddDynamic(this, &ULOL_HUDWidget::OnELevelUpClicked);
    }
    if (RLevelUpButton)
    {
        RLevelUpButton->OnClicked.RemoveAll(this);
        RLevelUpButton->OnClicked.AddDynamic(this, &ULOL_HUDWidget::OnRLevelUpClicked);
    }
}

void ULOL_HUDWidget::RequestSkillLevelUp(FName SkillName)
{
    if (ABaseChampion* Champion = Cast<ABaseChampion>(GetOwningPlayerPawn()))
    {
        if (Champion->SkillComponent)
        {
            Champion->SkillComponent->Server_LevelUpSkill(SkillName);
        }
    }
}

bool ULOL_HUDWidget::CanRequestSkillLevelUp(FName SkillName) const
{
    const ABaseChampion* Champion =
        Cast<ABaseChampion>(GetOwningPlayerPawn());
    const UChampion_SkillComponent* SkillComp =
        Champion ? Champion->SkillComponent : nullptr;

    return SkillComp && SkillComp->CanLevelUpSkill(SkillName);
}

void ULOL_HUDWidget::UpdateSkillLevelUpButtonStates()
{
    auto UpdateButton = [this](UWidget* Widget, UButton* Button, FName SkillName)
    {
        if (!Widget)
        {
            return;
        }

        const bool bCanLevelUp = CanRequestSkillLevelUp(SkillName);
        if (Button)
        {
            Button->SetIsEnabled(bCanLevelUp);
        }
        Widget->SetVisibility(
            bCanLevelUp
                ? ESlateVisibility::Visible
                : ESlateVisibility::Hidden);
    };

    UpdateButton(QLevelUpWidget, QLevelUpButton, TEXT("Q"));
    UpdateButton(WLevelUpWidget, WLevelUpButton, TEXT("W"));
    UpdateButton(ELevelUpWidget, ELevelUpButton, TEXT("E"));
    UpdateButton(RLevelUpWidget, RLevelUpButton, TEXT("R"));
}

void ULOL_HUDWidget::OnQLevelUpClicked()
{
    RequestSkillLevelUp(TEXT("Q"));
}

void ULOL_HUDWidget::OnWLevelUpClicked()
{
    RequestSkillLevelUp(TEXT("W"));
}

void ULOL_HUDWidget::OnELevelUpClicked()
{
    RequestSkillLevelUp(TEXT("E"));
}

void ULOL_HUDWidget::OnRLevelUpClicked()
{
    RequestSkillLevelUp(TEXT("R"));
}

void ULOL_HUDWidget::CacheScoreboardTextBlocks()
{
    if (!WidgetTree)
    {
        return;
    }

    auto FindTextBlock = [this](std::initializer_list<const TCHAR*> CandidateNames)
    {
        for (const TCHAR* CandidateName : CandidateNames)
        {
            if (UTextBlock* TextBlock =
                Cast<UTextBlock>(WidgetTree->FindWidget(FName(CandidateName))))
            {
                return TextBlock;
            }
        }

        return static_cast<UTextBlock*>(nullptr);
    };

    BlueKillCountText = FindTextBlock({ TEXT("blue_kill_count") });
    RedKillCountText = FindTextBlock({ TEXT("red_kill_count") });
    KillCountText = FindTextBlock({ TEXT("kills"), TEXT("kill_count") });
    DeathCountText = FindTextBlock({ TEXT("death"), TEXT("death_count") });
    AssistCountText = FindTextBlock({ TEXT("assists"), TEXT("assist_count") });
    KdaCountText = FindTextBlock({ TEXT("kda_count") });
    MinionCountText = FindTextBlock({ TEXT("minion_count"), TEXT("cs_count") });
    MatchTimerText = FindTextBlock({ TEXT("timer"), TEXT("time"), TEXT("times") });

}

void ULOL_HUDWidget::CacheMinimapChampionImages()
{
    CachedMinimapChampionImages.Reset();

    if (!WidgetTree)
    {
        return;
    }

    auto AddImage = [this](const FName& Name)
        {
            if (UImage* Img = Cast<UImage>(WidgetTree->FindWidget(Name)))
            {
                CachedMinimapChampionImages.Add(Img);
            }
            else
            {
                CachedMinimapChampionImages.Add(nullptr);
            }
        };

    // Minimap_Champion_Image (0번) 부터 Minimap_Champion_Image_9 (9번)까지 순서대로 매핑
    AddImage(TEXT("Minimap_Champion_Image"));
    AddImage(TEXT("Minimap_Champion_Image_1"));
    AddImage(TEXT("Minimap_Champion_Image_2"));
    AddImage(TEXT("Minimap_Champion_Image_3"));
    AddImage(TEXT("Minimap_Champion_Image_4"));
    AddImage(TEXT("Minimap_Champion_Image_5"));
    AddImage(TEXT("Minimap_Champion_Image_6"));
    AddImage(TEXT("Minimap_Champion_Image_7"));
    AddImage(TEXT("Minimap_Champion_Image_8"));
    AddImage(TEXT("Minimap_Champion_Image_9"));
}

void ULOL_HUDWidget::UpdateMinimapChampions()
{
    if (CachedMinimapChampionImages.Num() == 0)
    {
        CacheMinimapChampionImages();
    }

    if (!MiniMap)
    {
        return;
    }

    ALOL_PlayerController* PC = Cast<ALOL_PlayerController>(GetOwningPlayer());
    if (!PC)
    {
        return;
    }

    // 미니맵의 로컬 크기 가져오기
    const FVector2D MinimapSize = MiniMap->GetCachedGeometry().GetLocalSize();
    if (MinimapSize.X <= 0.f || MinimapSize.Y <= 0.f)
    {
        return;
    }

    // 현재 월드에 스폰되어 있는 모든 챔피언 수집
    TArray<AActor*> FoundChampions;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseChampion::StaticClass(), FoundChampions);

    // 슬롯 0 ~ 9번 순회
    for (int32 i = 0; i < CachedMinimapChampionImages.Num(); ++i)
    {
        UImage* SlotImage = CachedMinimapChampionImages[i];
        if (!SlotImage)
        {
            continue;
        }

        if (FoundChampions.IsValidIndex(i) && FoundChampions[i])
        {
            ABaseChampion* Champion = Cast<ABaseChampion>(FoundChampions[i]);
            if (Champion && Champion->ChampionResource.Portrait_Circle)
            {
                // 1. 챔피언의 월드 좌표 가져오기
                FVector ChampionLoc = Champion->GetActorLocation();

                // 2. 월드 좌표를 0.0 ~ 1.0 비율(UV)로 변환
                float NormX = FMath::GetRangePct(PC->MinimapWorldMin.X, PC->MinimapWorldMax.X, ChampionLoc.X);
                float NormY = FMath::GetRangePct(PC->MinimapWorldMin.Y, PC->MinimapWorldMax.Y, ChampionLoc.Y);

                NormX = FMath::Clamp(NormX, 0.0f, 1.0f);
                NormY = FMath::Clamp(NormY, 0.0f, 1.0f);

                // 미니맵 Y축 반전 옵션 적용
                if (PC->bInvertMinimapY)
                {
                    NormY = 1.0f - NormY;
                }

                // 3. 미니맵 내부에서의 상대 픽셀 위치 계산
                FVector2D LocalPosition(NormX * MinimapSize.X, NormY * MinimapSize.Y);

                // 캔버스 슬롯을 통해 미니맵 내부 상대 위치 설정
                if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotImage->Slot))
                {
                    CanvasSlot->SetPosition(LocalPosition);
                }

                // 초상화 적용 및 표시
                SlotImage->SetBrushFromTexture(Champion->ChampionResource.Portrait_Circle, false);
                SlotImage->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                SlotImage->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        else
        {
            SlotImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}
void ULOL_HUDWidget::UpdateHP(float NewHP, float MaxHP)
{
	if (HPProgressBar) HPProgressBar->SetPercent(NewHP / MaxHP);

    if (Txt_HP)
    {
        FText HPText = FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(FMath::FloorToInt(NewHP)),
            FText::AsNumber(FMath::FloorToInt(MaxHP))
        );

        Txt_HP->SetText(HPText);
    }
}
void ULOL_HUDWidget::UpdateMP(float NewMP, float MaxMP)
{
	if (MPProgressBar) MPProgressBar->SetPercent(NewMP / MaxMP);

    if (Txt_MP)
    {
        FText MPText = FText::Format(
            FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(FMath::FloorToInt(NewMP)),
            FText::AsNumber(FMath::FloorToInt(MaxMP))
        );

        Txt_MP->SetText(MPText);
    }
}

void ULOL_HUDWidget::UpdateEXP(float NewEXP, float MaxEXP)
{
    const float Percent = MaxEXP > 0.0f
        ? FMath::Clamp(NewEXP / MaxEXP, 0.0f, 1.0f)
        : 0.0f;
    ApplyEXPPercent(Percent);
}

void ULOL_HUDWidget::CacheEXPWidgets()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!EXPProgressBar)
    {
        const TArray<FName> EXPProgressBarNames = {
            TEXT("EXPProgressBar"),
            TEXT("ExpProgressBar"),
            TEXT("ExperienceBar"),
            TEXT("ExperienceProgressBar"),
            TEXT("exp_progressbar"),
            TEXT("exp_progress"),
            TEXT("EXP")
        };

        for (const FName& WidgetName : EXPProgressBarNames)
        {
            EXPProgressBar = Cast<UProgressBar>(WidgetTree->FindWidget(WidgetName));
            if (EXPProgressBar)
            {
                break;
            }
        }
    }

    if (!experiencebar)
    {
        experiencebar = Cast<UImage>(WidgetTree->FindWidget(TEXT("experiencebar")));
    }

    if (experiencebar)
    {
        EXP_MID = experiencebar->GetDynamicMaterial();
    }
}

void ULOL_HUDWidget::ApplyEXPProgressBarStyle()
{
    if (!EXPProgressBar || !EXPProgressFillSprite)
    {
        return;
    }

    FProgressBarStyle Style = EXPProgressBar->GetWidgetStyle();
    Style.FillImage.SetResourceObject(EXPProgressFillSprite);
    Style.FillImage.DrawAs = ESlateBrushDrawType::Image;
    Style.FillImage.Tiling = ESlateBrushTileType::NoTile;
    Style.FillImage.Mirroring = ESlateBrushMirrorType::NoMirror;

    EXPProgressBar->SetWidgetStyle(Style);
    EXPProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
}

void ULOL_HUDWidget::ApplyEXPPercent(float Percent)
{
    CurrentEXPPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

    if (EXPProgressBar)
    {
        EXPProgressBar->SetPercent(CurrentEXPPercent);
    }

    if (EXP_MID)
    {
        EXP_MID->SetScalarParameterValue(TEXT("newexpratio"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Percent"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("percent"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Progress"), CurrentEXPPercent);
        EXP_MID->SetScalarParameterValue(TEXT("Ratio"), CurrentEXPPercent);
    }
}

void ULOL_HUDWidget::SetSkillImage(FName SkillName, UTexture2D* IconTexture)
{
    UMaterialInstanceDynamic* TargetMID = nullptr;

    if (SkillName == TEXT("Q"))
    {
        TargetMID = SkillQ_MID;
    }
    else if (SkillName == TEXT("W"))
    {
        TargetMID = SkillW_MID;
    }
    else if (SkillName == TEXT("E"))
    {
        TargetMID = SkillE_MID;
    }
    else if (SkillName == TEXT("R"))
    {
        TargetMID = SkillR_MID;
    }
    if (TargetMID)
    {
        TargetMID->SetTextureParameterValue(TEXT("SkillIcon"), IconTexture);
    }
}

void ULOL_HUDWidget::AddItemIcon(UTexture2D* IconTexture)
{
    if (!IconTexture)
    {
        return;
    }

    if (CachedItemSlotImages.Num() == 0)
    {
        CacheItemSlotImages();
    }

    if (!CachedItemSlotImages.IsValidIndex(NextItemSlotIndex)) return;

    UImage* SlotImage = CachedItemSlotImages[NextItemSlotIndex];
    if (!SlotImage)
    {
        return;
    }

    FVector2D SlotSize = FVector2D::ZeroVector;
    if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotImage->Slot))
    {
        SlotSize = CanvasSlot->GetSize();
    }

    SlotImage->SetColorAndOpacity(FLinearColor::White);
    SlotImage->SetBrushFromTexture(IconTexture, false);

    FSlateBrush Brush = SlotImage->GetBrush();
    Brush.ImageSize = SlotSize.X > 0.f && SlotSize.Y > 0.f ? SlotSize : FVector2D(26.f, 26.f);
    SlotImage->SetBrush(Brush);
    ++NextItemSlotIndex;
}

void ULOL_HUDWidget::SetItemIcons(const TArray<UTexture2D*>& IconTextures)
{
    if (CachedItemSlotImages.Num() == 0)
    {
        CacheItemSlotImages();
    }

    NextItemSlotIndex = 0;

    for (UImage* SlotImage : CachedItemSlotImages)
    {
        if (SlotImage)
        {
            SlotImage->SetBrushFromTexture(nullptr);
            SlotImage->SetColorAndOpacity(FLinearColor::Transparent);
        }
    }

    for (UTexture2D* IconTexture : IconTextures)
    {
        AddItemIcon(IconTexture);
    }
}

FReply ULOL_HUDWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();

        if (MiniMap &&
            MiniMap->GetVisibility() == ESlateVisibility::Visible &&
            MiniMap->GetCachedGeometry()
            .IsUnderLocation(ScreenPosition))
        {
            const FGeometry Geometry =
                MiniMap->GetCachedGeometry();

            // 화면 좌표 -> 미니맵 내부 좌표
            const FVector2D LocalPosition =
                Geometry.AbsoluteToLocal(ScreenPosition);

            const FVector2D MinimapSize =
                Geometry.GetLocalSize();

            if (MinimapSize.X > 0.f && MinimapSize.Y > 0.f)
            {
                // 내부 좌표 -> 0~1 UV 좌표
                const FVector2D UV(
                    LocalPosition.X / MinimapSize.X,
                    LocalPosition.Y / MinimapSize.Y
                );

                if (ALOL_PlayerController* PC =
                    Cast<ALOL_PlayerController>(GetOwningPlayer()))
                {
                    PC->MoveCameraToMinimapUV(UV);
                }
            }

            // 미니맵 클릭을 챔피언 이동이나 다른 UI에 전달하지 않음
            return FReply::Handled();
        }

        auto TryClickSkillLevelUpWidget =
            [this, ScreenPosition](UWidget* Widget, FName SkillName)
        {
            if (!Widget ||
                Widget->GetVisibility() != ESlateVisibility::Visible ||
                !Widget->GetCachedGeometry().IsUnderLocation(ScreenPosition) ||
                !CanRequestSkillLevelUp(SkillName))
            {
                return false;
            }

            RequestSkillLevelUp(SkillName);
            Widget->SetVisibility(ESlateVisibility::Hidden);
            return true;
        };

        if (TryClickSkillLevelUpWidget(QLevelUpWidget, TEXT("Q")) ||
            TryClickSkillLevelUpWidget(WLevelUpWidget, TEXT("W")) ||
            TryClickSkillLevelUpWidget(ELevelUpWidget, TEXT("E")) ||
            TryClickSkillLevelUpWidget(RLevelUpWidget, TEXT("R")))
        {
            return FReply::Handled();
        }

        if (SummonerSpell1Image &&
            SummonerSpell1Image->GetCachedGeometry().IsUnderLocation(ScreenPosition))
        {
            if (ALOL_PlayerController* PlayerController =
                Cast<ALOL_PlayerController>(GetOwningPlayer()))
            {
                PlayerController->OnSummonerSpell1();
                return FReply::Handled();
            }
        }

        if (SummonerSpell2Image &&
            SummonerSpell2Image->GetCachedGeometry().IsUnderLocation(ScreenPosition))
        {
            if (ALOL_PlayerController* PlayerController =
                Cast<ALOL_PlayerController>(GetOwningPlayer()))
            {
                PlayerController->OnSummonerSpell2();
                return FReply::Handled();
            }
        }

        for (int32 SlotIndex = 0; SlotIndex < CachedItemSlotImages.Num(); ++SlotIndex)
        {
            UImage* SlotImage = CachedItemSlotImages[SlotIndex];
            if (SlotImage &&
                SlotImage->GetVisibility() == ESlateVisibility::Visible &&
                SlotImage->GetCachedGeometry().IsUnderLocation(ScreenPosition))
            {
                if (ALOL_PlayerController* PlayerController =
                    Cast<ALOL_PlayerController>(GetOwningPlayer()))
                {
                    PlayerController->SelectInventoryItem(SlotIndex);
                    return FReply::Handled();
                }
            }
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULOL_HUDWidget::CacheItemSlotImages()
{
    CachedItemSlotImages.Reset();

    if (!WidgetTree)
    {
        return;
    }

    auto AddItemSlotIfValid = [this](UImage* SlotImage, const TCHAR* ExpectedName)
    {
        if (!SlotImage)
        {
            return;
        }

        if (SlotImage == SkillQ_Image || SlotImage == SkillW_Image || SlotImage == SkillE_Image || SlotImage == SkillR_Image || SlotImage == SkillP_Image)
        {
            return;
        }

        CachedItemSlotImages.Add(SlotImage);
    };

    AddItemSlotIfValid(ItemSlot_1, TEXT("ItemSlot_1"));
    AddItemSlotIfValid(ItemSlot_2, TEXT("ItemSlot_2"));
    AddItemSlotIfValid(ItemSlot_3, TEXT("ItemSlot_3"));
    AddItemSlotIfValid(ItemSlot_4, TEXT("ItemSlot_4"));
    AddItemSlotIfValid(ItemSlot_5, TEXT("ItemSlot_5"));
    AddItemSlotIfValid(ItemSlot_6, TEXT("ItemSlot_6"));

    if (CachedItemSlotImages.Num() > 0)
    {
        return;
    }

    const TArray<FName> PreferredSlotNames =
    {
        FName("ItemSlot_1"),
        FName("ItemSlot_2"),
        FName("ItemSlot_3"),
        FName("ItemSlot_4"),
        FName("ItemSlot_5"),
        FName("ItemSlot_6")
    };

    for (const FName& SlotName : PreferredSlotNames)
    {
        if (UImage* SlotImage = Cast<UImage>(WidgetTree->FindWidget(SlotName)))
        {
            AddItemSlotIfValid(SlotImage, *SlotName.ToString());
        }
    }

    if (CachedItemSlotImages.Num() > 0)
    {
        return;
    }
}

void ULOL_HUDWidget::CacheSummonerSpellWidgets()
{
    if (!WidgetTree)
    {
        return;
    }

    SummonerSpell1Image = Cast<UImage>(WidgetTree->FindWidget(TEXT("spell1")));
    SummonerSpell2Image = Cast<UImage>(WidgetTree->FindWidget(TEXT("spell2")));

    if (!SummonerSpell1Image)
    {
        SummonerSpell1Image = Cast<UImage>(WidgetTree->FindWidget(TEXT("spell1_slot")));
    }
    if (!SummonerSpell2Image)
    {
        SummonerSpell2Image = Cast<UImage>(WidgetTree->FindWidget(TEXT("spell2_slot")));
    }

    auto InitializeSpellImage = [this](
        UImage* SpellImage,
        UTexture2D* Icon,
        UMaterialInstanceDynamic*& OutMID)
    {
        if (!SpellImage || !Icon)
        {
            return;
        }

        if (SkillCooldownMaterial)
        {
            SpellImage->SetBrushFromMaterial(SkillCooldownMaterial);
            OutMID = SpellImage->GetDynamicMaterial();
        }

        if (OutMID)
        {
            OutMID->SetTextureParameterValue(TEXT("SkillIcon"), Icon);
            OutMID->SetScalarParameterValue(TEXT("Cooldown"), 0.0f);
        }
        else
        {
            SpellImage->SetBrushFromTexture(Icon, false);
        }
    };

    InitializeSpellImage(SummonerSpell1Image, ExhaustIcon, SummonerSpell1_MID);
    InitializeSpellImage(SummonerSpell2Image, FlashIcon, SummonerSpell2_MID);
}

void ULOL_HUDWidget::SetSkillCooldown(FName SkillName, float CoolLocalEndTime, float CoolEndTime)
{
    if (SkillName == "Q")
    {
        SkillCoolLocalEndTimeQ = CoolLocalEndTime;
        SkillCoolEndTimeQ = CoolEndTime;
    }
    else if (SkillName == "W")
    {
        SkillCoolLocalEndTimeW = CoolLocalEndTime;
        SkillCoolEndTimeW = CoolEndTime;
    }
    else if (SkillName == "E")
    {
        SkillCoolLocalEndTimeE = CoolLocalEndTime;
        SkillCoolEndTimeE = CoolEndTime;
    }
    else if (SkillName == "R")
    {
        SkillCoolLocalEndTimeR = CoolLocalEndTime;
        SkillCoolEndTimeR = CoolEndTime;
    }
    else if (SkillName == "P")
    {
        SkillCoolLocalEndTimeP = CoolLocalEndTime;
        SkillCoolEndTimeP = CoolEndTime;
    }
    else if (SkillName == "Spell1")
    {
        SummonerSpell1CoolLocalEndTime = CoolLocalEndTime;
        SummonerSpell1CoolEndTime = CoolEndTime;
    }
    else if (SkillName == "Spell2")
    {
        SummonerSpell2CoolLocalEndTime = CoolLocalEndTime;
        SummonerSpell2CoolEndTime = CoolEndTime;
    }
}

