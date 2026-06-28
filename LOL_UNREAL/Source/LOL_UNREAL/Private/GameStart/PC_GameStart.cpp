#include "GameStart/PC_GameStart.h"

#include "Lobby/LOL_GameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/SlateTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName HostButtonName(TEXT("\uAC8C\uC784\uC2DC\uC791\uBC84\uD2BC"));
	const FName JoinButtonName(TEXT("\uAC8C\uC784\uC2DC\uC791\uBC84\uD2BC_1"));
	const FName NicknameTextName(TEXT("text_id"));
	const FName IpTextName(TEXT("text_ip"));
	const FName IpOverlayName(TEXT("Overlay_0"));
	const FName LobbyMapName(TEXT("/Game/Lobby/Lobby"));
}

APC_GameStart::APC_GameStart()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> GameStartWidgetFinder(
		TEXT("/Game/UI/wbp_gamestart"));

	if (GameStartWidgetFinder.Succeeded())
	{
		GameStartWidgetClass = GameStartWidgetFinder.Class;
	}
}

void APC_GameStart::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!GameStartWidgetClass)
	{
		ShowInputError(TEXT("GameStart widget class was not found: /Game/UI/wbp_gamestart"));
		return;
	}

	GameStartWidget = CreateWidget<UUserWidget>(this, GameStartWidgetClass);
	if (!GameStartWidget)
	{
		ShowInputError(TEXT("Failed to create wbp_gamestart."));
		return;
	}

	GameStartWidget->AddToViewport();
	BindGameStartWidget();

	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	if (NicknameInput)
	{
		InputMode.SetWidgetToFocus(NicknameInput->TakeWidget());
	}
	SetInputMode(InputMode);

	if (NicknameInput)
	{
		NicknameInput->SetFocus();
	}
}

void APC_GameStart::BindGameStartWidget()
{
	if (!GameStartWidget || !GameStartWidget->WidgetTree)
	{
		return;
	}

	UButton* HostButton = Cast<UButton>(GameStartWidget->WidgetTree->FindWidget(HostButtonName));
	UButton* JoinButton = Cast<UButton>(GameStartWidget->WidgetTree->FindWidget(JoinButtonName));
	IpInputOverlay = Cast<UOverlay>(GameStartWidget->WidgetTree->FindWidget(IpOverlayName));

	NicknameInput = ReplaceTextBlockWithEditable(
		Cast<UTextBlock>(GameStartWidget->WidgetTree->FindWidget(NicknameTextName)),
		TEXT("NicknameInput"));
	IpAddressInput = ReplaceTextBlockWithEditable(
		Cast<UTextBlock>(GameStartWidget->WidgetTree->FindWidget(IpTextName)),
		TEXT("IpAddressInput"));

	if (ULOL_GameInstance* GameInstance = GetGameInstance<ULOL_GameInstance>())
	{
		if (NicknameInput && !GameInstance->MySavedNickname.IsEmpty()
			&& GameInstance->MySavedNickname != TEXT("test"))
		{
			NicknameInput->SetText(FText::FromString(GameInstance->MySavedNickname));
		}
	}

	if (IpInputOverlay)
	{
		IpInputOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &APC_GameStart::HandleHostClicked);
	}
	else
	{
		ShowInputError(TEXT("Host button was not found in wbp_gamestart."));
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &APC_GameStart::HandleJoinClicked);
	}
	else
	{
		ShowInputError(TEXT("Join button was not found in wbp_gamestart."));
	}

	if (IpAddressInput)
	{
		IpAddressInput->OnTextCommitted.AddDynamic(this, &APC_GameStart::HandleIpTextCommitted);
	}
}

UEditableTextBox* APC_GameStart::ReplaceTextBlockWithEditable(
	UTextBlock* OriginalText,
	FName EditableName)
{
	if (!OriginalText || !GameStartWidget || !GameStartWidget->WidgetTree)
	{
		ShowInputError(FString::Printf(TEXT("Input placeholder was not found: %s"), *EditableName.ToString()));
		return nullptr;
	}

	UPanelWidget* Parent = OriginalText->GetParent();
	if (!Parent)
	{
		ShowInputError(FString::Printf(TEXT("Input placeholder has no parent: %s"), *EditableName.ToString()));
		return nullptr;
	}

	UEditableTextBox* Editable = GameStartWidget->WidgetTree->ConstructWidget<UEditableTextBox>(
		UEditableTextBox::StaticClass(),
		EditableName);
	if (!Editable)
	{
		return nullptr;
	}

	Editable->SetHintText(OriginalText->GetText());
	Editable->SetJustification(ETextJustify::Center);
	Editable->SetClearKeyboardFocusOnCommit(false);
	Editable->SetSelectAllTextWhenFocused(false);

	FEditableTextBoxStyle Style = Editable->GetWidgetStyle();
	Style.SetFont(OriginalText->GetFont());
	Style.SetForegroundColor(OriginalText->GetColorAndOpacity());
	Style.SetFocusedForegroundColor(OriginalText->GetColorAndOpacity());
	Style.SetPadding(FMargin(0.0f));
	Style.BackgroundImageNormal.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.BackgroundImageHovered.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.BackgroundImageFocused.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.BackgroundImageReadOnly.DrawAs = ESlateBrushDrawType::NoDrawType;
	Editable->SetWidgetStyle(Style);

	if (UCanvasPanel* CanvasParent = Cast<UCanvasPanel>(Parent))
	{
		const UCanvasPanelSlot* OriginalSlot = Cast<UCanvasPanelSlot>(OriginalText->Slot);
		const FAnchorData Layout = OriginalSlot ? OriginalSlot->GetLayout() : FAnchorData();
		const bool bAutoSize = OriginalSlot && OriginalSlot->GetAutoSize();
		const int32 ZOrder = OriginalSlot ? OriginalSlot->GetZOrder() : 1;

		OriginalText->RemoveFromParent();
		if (UCanvasPanelSlot* NewSlot = CanvasParent->AddChildToCanvas(Editable))
		{
			NewSlot->SetLayout(Layout);
			NewSlot->SetAutoSize(bAutoSize);
			NewSlot->SetZOrder(ZOrder);
		}
		Editable->SetMinDesiredWidth(520.0f);
	}
	else if (UOverlay* OverlayParent = Cast<UOverlay>(Parent))
	{
		const UOverlaySlot* OriginalSlot = Cast<UOverlaySlot>(OriginalText->Slot);
		const FMargin Padding = OriginalSlot ? OriginalSlot->GetPadding() : FMargin();
		const EHorizontalAlignment HorizontalAlignment =
			OriginalSlot ? OriginalSlot->GetHorizontalAlignment() : HAlign_Fill;
		const EVerticalAlignment VerticalAlignment =
			OriginalSlot ? OriginalSlot->GetVerticalAlignment() : VAlign_Fill;

		OriginalText->RemoveFromParent();
		if (UOverlaySlot* NewSlot = OverlayParent->AddChildToOverlay(Editable))
		{
			NewSlot->SetPadding(Padding);
			NewSlot->SetHorizontalAlignment(HorizontalAlignment);
			NewSlot->SetVerticalAlignment(VerticalAlignment);
		}
		Editable->SetMinDesiredWidth(430.0f);
	}
	else
	{
		ShowInputError(FString::Printf(
			TEXT("Unsupported input parent for %s: %s"),
			*EditableName.ToString(),
			*Parent->GetClass()->GetName()));
		return nullptr;
	}

	return Editable;
}

void APC_GameStart::HandleHostClicked()
{
	if (!SaveNickname())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Creating Lobby listen server. Nickname=%s"),
		*GetGameInstance<ULOL_GameInstance>()->MySavedNickname);
	UGameplayStatics::OpenLevel(this, LobbyMapName, true, TEXT("listen"));
}

void APC_GameStart::HandleJoinClicked()
{
	if (!IpInputOverlay || !IpAddressInput)
	{
		ShowInputError(TEXT("IP input UI is not available."));
		return;
	}

	if (IpInputOverlay->GetVisibility() != ESlateVisibility::Visible)
	{
		IpInputOverlay->SetVisibility(ESlateVisibility::Visible);
		IpAddressInput->SetFocus();
		return;
	}

	JoinLobby();
}

void APC_GameStart::HandleIpTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		JoinLobby();
	}
}

bool APC_GameStart::SaveNickname()
{
	if (!NicknameInput)
	{
		ShowInputError(TEXT("Nickname input is not available."));
		return false;
	}

	FString Nickname = NicknameInput->GetText().ToString();
	Nickname.TrimStartAndEndInline();
	if (Nickname.IsEmpty())
	{
		NicknameInput->SetError(FText::FromString(TEXT("Enter a nickname.")));
		ShowInputError(TEXT("A nickname is required."));
		NicknameInput->SetFocus();
		return false;
	}

	NicknameInput->ClearError();
	if (ULOL_GameInstance* GameInstance = GetGameInstance<ULOL_GameInstance>())
	{
		GameInstance->MySavedNickname = Nickname.Left(20);
		return true;
	}

	ShowInputError(TEXT("LOL_GameInstance is not active."));
	return false;
}

void APC_GameStart::JoinLobby()
{
	if (!SaveNickname() || !IpAddressInput)
	{
		return;
	}

	FString Address = IpAddressInput->GetText().ToString();
	Address.TrimStartAndEndInline();
	Address.RemoveFromStart(TEXT("open "));

	if (Address.IsEmpty())
	{
		IpAddressInput->SetError(FText::FromString(TEXT("Enter the server IP address.")));
		ShowInputError(TEXT("A server IP address is required."));
		IpAddressInput->SetFocus();
		return;
	}

	IpAddressInput->ClearError();
	if (ULOL_GameInstance* GameInstance = GetGameInstance<ULOL_GameInstance>())
	{
		GameInstance->BeginLobbyJoin(Address);
	}

	UE_LOG(LogTemp, Log, TEXT("Joining Lobby server. Address=%s"), *Address);
	ClientTravel(Address, TRAVEL_Absolute);
}

void APC_GameStart::ShowInputError(const FString& Message) const
{
	UE_LOG(LogTemp, Error, TEXT("GameStart: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red, Message);
	}
}
