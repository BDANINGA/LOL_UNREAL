// HUD 관리하는 위젯 클래스
#include "Widget/LOL_HUDWidget.h"
#include "LOL_PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Blueprint/UserWidget.h"
#include "LOL_GameState.h"
#include "LOL_PlayerState.h"
#include "BaseChampion.h"
#include "GameFramework/PlayerController.h"
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
}

void ULOL_HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SkillQ_Image) SkillQ_MID = SkillQ_Image->GetDynamicMaterial();
    if (SkillW_Image) SkillW_MID = SkillW_Image->GetDynamicMaterial();
    if (SkillE_Image) SkillE_MID = SkillE_Image->GetDynamicMaterial();
    if (SkillR_Image) SkillR_MID = SkillR_Image->GetDynamicMaterial();

    CacheItemSlotImages();
    CacheScoreboardTextBlocks();
    CreateKillLogContainer();

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

    UpdateScoreboard();
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
        UE_LOG(LogTemp, Warning, TEXT("Kill log container could not be created: HUD root is not a CanvasPanel."));
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

    UE_LOG(
        LogTemp,
        Log,
        TEXT("HUD scoreboard widgets: Blue=%s Red=%s K=%s D=%s A=%s KDA=%s CS=%s Time=%s"),
        BlueKillCountText ? *BlueKillCountText->GetName() : TEXT("None"),
        RedKillCountText ? *RedKillCountText->GetName() : TEXT("None"),
        KillCountText ? *KillCountText->GetName() : TEXT("None"),
        DeathCountText ? *DeathCountText->GetName() : TEXT("None"),
        AssistCountText ? *AssistCountText->GetName() : TEXT("None"),
        KdaCountText ? *KdaCountText->GetName() : TEXT("None"),
        MinionCountText ? *MinionCountText->GetName() : TEXT("None"),
        MatchTimerText ? *MatchTimerText->GetName() : TEXT("None"));
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
    /*if (EXPProgressBar) MPProgressBar->SetPercent(NewEXP / MaxEXP);*/
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

    if (!CachedItemSlotImages.IsValidIndex(NextItemSlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("HUD item slot is full or not found. SlotIndex=%d SlotCount=%d"), NextItemSlotIndex, CachedItemSlotImages.Num());
        return;
    }

    UImage* SlotImage = CachedItemSlotImages[NextItemSlotIndex];
    if (!SlotImage)
    {
        return;
    }

    FVector2D SlotPosition = FVector2D::ZeroVector;
    FVector2D SlotSize = FVector2D::ZeroVector;
    if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SlotImage->Slot))
    {
        SlotPosition = CanvasSlot->GetPosition();
        SlotSize = CanvasSlot->GetSize();
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("HUD item icon set. SlotIndex=%d Widget=%s CanvasPos=(%.1f, %.1f) CanvasSize=(%.1f, %.1f)"),
        NextItemSlotIndex,
        *SlotImage->GetName(),
        SlotPosition.X,
        SlotPosition.Y,
        SlotSize.X,
        SlotSize.Y
    );

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
            UE_LOG(LogTemp, Warning, TEXT("HUD item slot missing. Expected=%s"), ExpectedName);
            return;
        }

        if (SlotImage == SkillQ_Image || SlotImage == SkillW_Image || SlotImage == SkillE_Image || SlotImage == SkillR_Image || SlotImage == SkillP_Image)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("HUD item slot rejected because it is a skill image. Expected=%s ActualWidget=%s"),
                ExpectedName,
                *SlotImage->GetName()
            );
            return;
        }

        CachedItemSlotImages.Add(SlotImage);
        UE_LOG(LogTemp, Log, TEXT("HUD item slot accepted. Expected=%s ActualWidget=%s"), ExpectedName, *SlotImage->GetName());
    };

    AddItemSlotIfValid(ItemSlot_1, TEXT("ItemSlot_1"));
    AddItemSlotIfValid(ItemSlot_2, TEXT("ItemSlot_2"));
    AddItemSlotIfValid(ItemSlot_3, TEXT("ItemSlot_3"));
    AddItemSlotIfValid(ItemSlot_4, TEXT("ItemSlot_4"));
    AddItemSlotIfValid(ItemSlot_5, TEXT("ItemSlot_5"));
    AddItemSlotIfValid(ItemSlot_6, TEXT("ItemSlot_6"));

    if (CachedItemSlotImages.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("HUD item slots cached by explicit bind. Count=%d"), CachedItemSlotImages.Num());
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
        UE_LOG(LogTemp, Log, TEXT("HUD item slots cached by name. Count=%d"), CachedItemSlotImages.Num());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HUD item slots not found. Rename the six small item icon Images to ItemSlot_1 through ItemSlot_6."));
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
}

