// Fill out your copyright notice in the Description page of Project Settings.
// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// 1. 이동
// 2. 공격
// ---------------------------------------------------------------------------

#include "LOL_PlayerController.h"
#include "LOL_HUD.h"
#include "BaseChampion.h"
#include "Minion/BaseMinion.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Camera.h"

#include "Widget/LOL_CursorWidget.h"

#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_UIComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Item/LOL_ItemData.h"

#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h" 
#include "NiagaraSystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ContentWidget.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void ULOL_ShopButtonBinding::Initialize(ALOL_PlayerController* InOwnerController, FName InItemName, bool bInBuyButton, bool bInSellButton)
{
	OwnerController = InOwnerController;
	ItemName = InItemName;
	bBuyButton = bInBuyButton;
	bSellButton = bInSellButton;
}

void ULOL_ShopButtonBinding::HandleClicked()
{
	if (!OwnerController)
	{
		return;
	}

	if (bBuyButton)
	{
		OwnerController->BuySelectedShopItem();
		return;
	}

	if (bSellButton)
	{
		OwnerController->SellSelectedShopItem();
		return;
	}

	OwnerController->SelectShopItem(ItemName);
}

ALOL_PlayerController::ALOL_PlayerController()
{
	bAutoManageActiveCameraTarget = false;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Default(TEXT("/Game/Level/input/IMC_Default.IMC_Default"));
	if (IMC_Default.Succeeded()) DefaultMappingContext = IMC_Default.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_RightClick(TEXT("/Game/Level/input/IA_RightClick.IA_RightClick"));
	if (IA_RightClick.Succeeded()) RightClickAction = IA_RightClick.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_LeftClick(TEXT("/Game/Level/input/IA_LeftClick.IA_LeftClick"));
	if (IA_LeftClick.Succeeded()) LeftClickAction = IA_LeftClick.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillQ(TEXT("/Game/Level/input/IA_SkillQ.IA_SkillQ"));
	if (IA_SkillQ.Succeeded()) SkillQAction = IA_SkillQ.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillW(TEXT("/Game/Level/input/IA_SkillW.IA_SkillW"));
	if (IA_SkillW.Succeeded()) SkillWAction = IA_SkillW.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillE(TEXT("/Game/Level/input/IA_SkillE.IA_SkillE"));
	if (IA_SkillE.Succeeded()) SkillEAction = IA_SkillE.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillR(TEXT("/Game/Level/input/IA_SkillR.IA_SkillR"));
	if (IA_SkillR.Succeeded()) SkillRAction = IA_SkillR.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SpaceBar(TEXT("/Game/Level/input/IA_SpaceBar.IA_SpaceBar"));
	if (IA_SpaceBar.Succeeded()) SpaceBarAction = IA_SpaceBar.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_AKey(TEXT("/Game/Level/input/IA_A.IA_A"));
	if (IA_AKey.Succeeded()) AKeyAction = IA_AKey.Object;

	static ConstructorHelpers::FClassFinder<ULOL_CursorWidget> CursorWidgetAsset(TEXT("/Game/UI/Cursor/Wbp_CursorWidget.Wbp_CursorWidget_C"));
	if (CursorWidgetAsset.Succeeded()) CursorWidgetClass = CursorWidgetAsset.Class;

	static ConstructorHelpers::FClassFinder<UUserWidget> ShopWidgetAsset(TEXT("/Game/UI/wbp_shop.wbp_shop_C"));
	if (ShopWidgetAsset.Succeeded()) ShopWidgetClass = ShopWidgetAsset.Class;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXAsset(TEXT("/Game/UI/Cursor/Indicator/FX_ClickIndicator.FX_ClickIndicator"));
	if (FXAsset.Succeeded()) ClickFX = FXAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXAsset1(TEXT("/Game/UI/Cursor/Indicator/FX_AttackIndicator.FX_AttackIndicator"));
	if (FXAsset1.Succeeded()) AClickFX = FXAsset1.Object;

	ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/LOL_Data/Data_Items/Data_ItemStats.Data_ItemStats"));
	if (!ItemDataTable)
	{
		ItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/LOL_Data/Data_Champions/Data_ItemStats.Data_ItemStats"));
	}

	ShopActorTags.Add(FName("Shop"));
	ShopActorTags.Add(FName("shop"));
	ShopActorTags.Add(FName("ItemShop"));
}

void ALOL_PlayerController::SetSelectedChampionClass(
	TSubclassOf<ABaseChampion> InChampionClass)
{
	if (HasAuthority())
	{
		SelectedChampionClass = InChampionClass;
	}
}

void ALOL_PlayerController::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALOL_PlayerController, SelectedChampionClass);
}

void ALOL_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (CursorWidgetClass)
		{
			MyCursorWidget = CreateWidget<ULOL_CursorWidget>(this, CursorWidgetClass);
			if (MyCursorWidget)
			{
				SetMouseCursorWidget(EMouseCursor::Default, MyCursorWidget);
			}
		}

	}
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	CameraAnchor = GetWorld()->SpawnActor<ACamera>(ACamera::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (CameraAnchor)
	{
		CameraAnchor->SetFollowTarget(GetPawn());
		SetViewTarget(CameraAnchor);
	}
}
void ALOL_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	MyChampion = Cast<ABaseChampion>(InPawn);
}

void ALOL_PlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	MyChampion = Cast<ABaseChampion>(GetPawn());
}
void ALOL_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController())
	{
		UpdateCursorSelection();
		FreeCameraEdgeScroll(DeltaTime);
	}

	FreeCameraEdgeScroll(DeltaTime);
}

void ALOL_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnRightClick);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnLeftClick);
		EnhancedInputComponent->BindAction(SkillQAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(SkillWAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(SkillRAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillR);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnToggleCamera);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Completed, this, &ALOL_PlayerController::OnToggleCamera);
		EnhancedInputComponent->BindAction(AKeyAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnAKey);
	}

	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ALOL_PlayerController::OnToggleShop);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ALOL_PlayerController::OnRecall);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALOL_PlayerController::OnQuitGame);
}

void ALOL_PlayerController::OnQuitGame()
{
	if (!IsLocalController())
	{
		return;
	}

	UKismetSystemLibrary::QuitGame(
		this,
		this,
		EQuitPreference::Quit,
		false
	);
}

void ALOL_PlayerController::OnRecall()
{
	ABaseChampion* Champion = MyChampion ? MyChampion : Cast<ABaseChampion>(GetPawn());
	if (Champion)
	{
		Champion->StartRecall();
	}
}

void ALOL_PlayerController::OnRightClick()
{
	FHitResult HitResult;
	if (GetTargetAwareHitUnderCursor(HitResult))
	{
		AActor* HitActor = HitResult.GetActor();
		if (MyChampion && MyChampion->IsLocallyControlled())
		{
			MyChampion->SetIsPressA(false);
			MyChampion->MoveComponent->bIsSearchAttack = false;
			MyChampion->UIComponent->HideRangeIndicator();
			MyChampion->ProcessMoveInput(HitResult.Location, HitActor);

			if (!IsClickableAttackTarget(HitActor))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ClickFX,
					HitResult.Location + FVector(0.f, 0.f, 20.f),
					FRotator(-90.f, 0.f, 0.f)
				);
			}
		}
	}
}
void ALOL_PlayerController::OnLeftClick()
{
	if (MyChampion && MyChampion->IsLocallyControlled())
	{
		if (MyChampion->GetIsPressA())
		{
			FHitResult HitResult;
			
			if (GetTargetAwareHitUnderCursor(HitResult))
			{
				AActor* HitActor = HitResult.GetActor();
				MyChampion->ProcessMoveInput(HitResult.Location, HitActor);
				MyChampion->SetIsPressA(false);
				MyChampion->MoveComponent->bIsSearchAttack = true;
				MyChampion->UIComponent->HideRangeIndicator();

				if (!IsClickableAttackTarget(HitActor))
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						AClickFX,
						HitResult.Location + FVector(0.f, 0.f, 20.f),
						FRotator(-90.f, 0.f, 0.f)
					);
				}
			}
		}
	}
}
void ALOL_PlayerController::OnToggleCamera()
{
	if (CameraAnchor)
	{
		bool bNewLock = !CameraAnchor->IsLocked();
		CameraAnchor->SetCameraLock(bNewLock);

		if (bNewLock)
		{
			CameraAnchor->SetFollowTarget(GetPawn());
		}
	}
}

void ALOL_PlayerController::FreeCameraEdgeScroll(float DeltaTime)
{
	if (!CameraAnchor || CameraAnchor->IsLocked()) return;

	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		FVector MoveDir = FVector::ZeroVector;
		float EdgeThreshold = 10.0f; // 가장자리 인식 범위

		// 8방향 체크 로직
		if (MouseX <= EdgeThreshold) MoveDir.X = -1; // 왼쪽
		else if (MouseX >= ViewportSizeX - EdgeThreshold) MoveDir.X = 1; // 오른쪽

		if (MouseY <= EdgeThreshold) MoveDir.Y = -1;
		else if (MouseY >= ViewportSizeY - EdgeThreshold) MoveDir.Y = 1; // 아래

		if (!MoveDir.IsZero())
		{
			MoveDir.Normalize();
			CameraAnchor->MoveAnchor(MoveDir, DeltaTime);
		}
	}
}
void ALOL_PlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalPlayerController())
	{
		InitCameraAnchor(P);
	}
}
void ALOL_PlayerController::InitCameraAnchor(APawn* TargetPawn)
{
	if (!TargetPawn) return;

	if (CameraAnchor)
	{
		CameraAnchor->Destroy();
		CameraAnchor = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	FVector ChampionLocation = TargetPawn->GetActorLocation();
	CameraAnchor = GetWorld()->SpawnActor<ACamera>(ACamera::StaticClass(), 
		ChampionLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (CameraAnchor)
	{
		CameraAnchor->SetFollowTarget(TargetPawn);
		SetViewTarget(CameraAnchor);
	}
}

void ALOL_PlayerController::OnSkillQ()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('q');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
}
void ALOL_PlayerController::OnSkillW()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('w');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
}
void ALOL_PlayerController::OnSkillE()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('e');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
}
void ALOL_PlayerController::OnSkillR()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('r');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
}
void ALOL_PlayerController::OnAKey()
{
	if (MyChampion)
	{
		MyChampion->SetIsPressA(true);
		MyChampion->UIComponent->ShowRangeIndicator();
	}
}

void ALOL_PlayerController::OnToggleShop()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!ShopWidget && ShopWidgetClass)
	{
		ShopWidget = CreateWidget<UUserWidget>(this, ShopWidgetClass);
		bShopButtonsBound = false;
	}

	if (!ShopWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop widget class is not set. Expected /Game/UI/wbp_shop.wbp_shop_C"));
		return;
	}

	if (ShopWidget->IsInViewport())
	{
		SelectedShopItemName = NAME_None;
		SelectedInventoryItemSlotIndex = INDEX_NONE;
		ShopWidget->RemoveFromParent();
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		return;
	}

	BindShopWidgetButtons();
	SelectedShopItemName = NAME_None;
	SelectedInventoryItemSlotIndex = INDEX_NONE;
	ShopWidget->AddToViewport();
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ShopWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALOL_PlayerController::SelectShopItem(FName ItemName)
{
	if (ItemName.IsNone())
	{
		return;
	}

	SelectedShopItemName = ItemName;
	UE_LOG(LogTemp, Log, TEXT("Shop item selected. Item=%s"), *SelectedShopItemName.ToString());
}

void ALOL_PlayerController::SelectInventoryItem(int32 ItemSlotIndex)
{
	if (!ShopWidget || !ShopWidget->IsInViewport())
	{
		return;
	}

	if (!PurchasedItemNames.IsValidIndex(ItemSlotIndex))
	{
		SelectedInventoryItemSlotIndex = INDEX_NONE;
		return;
	}

	SelectedInventoryItemSlotIndex = ItemSlotIndex;
}

void ALOL_PlayerController::BuySelectedShopItem()
{
	if (SelectedShopItemName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Buy item failed. No shop item selected."));
		return;
	}

	const FName ItemToBuy = SelectedShopItemName;
	SelectedShopItemName = NAME_None;
	RequestBuyItem(ItemToBuy);
}

void ALOL_PlayerController::SellSelectedShopItem()
{
	if (!PurchasedItemNames.IsValidIndex(SelectedInventoryItemSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Sell item failed. No inventory item selected."));
		return;
	}

	const int32 ItemSlotIndexToSell = SelectedInventoryItemSlotIndex;
	SelectedInventoryItemSlotIndex = INDEX_NONE;
	Server_SellItem(ItemSlotIndexToSell);
}

void ALOL_PlayerController::RequestBuyItem(FName ItemName)
{
	if (ItemName.IsNone())
	{
		return;
	}

	Server_BuyItem(ItemName);
}

void ALOL_PlayerController::Server_BuyItem_Implementation(FName ItemName)
{
	ABaseChampion* Champion = MyChampion ? MyChampion : Cast<ABaseChampion>(GetPawn());
	if (!Champion || !Champion->StatComponent)
	{
		return;
	}

	if (!IsInShopRange())
	{
		UE_LOG(LogTemp, Warning, TEXT("Buy item failed. Champion is not in shop range. Item=%s"), *ItemName.ToString());
		return;
	}

	FItemData ItemData;
	if (!FindItemData(ItemName, ItemData))
	{
		UE_LOG(LogTemp, Warning, TEXT("Buy item failed. Item row not found. Item=%s"), *ItemName.ToString());
		return;
	}

	struct FConsumedRecipeItem
	{
		int32 InventoryIndex = INDEX_NONE;
		FItemData ItemData;
	};

	TArray<FConsumedRecipeItem> ConsumedRecipeItems;
	TSet<int32> UsedInventoryIndices;
	TSet<FName> ActiveRecipePath;
	int32 ConsumedRecipeValue = 0;

	TFunction<void(FName)> CollectOwnedRecipeItems;
	CollectOwnedRecipeItems =
		[&](FName RecipeItemName)
	{
		for (int32 InventoryIndex = 0;
			InventoryIndex < PurchasedItemNames.Num();
			++InventoryIndex)
		{
			if (UsedInventoryIndices.Contains(InventoryIndex) ||
				PurchasedItemNames[InventoryIndex] != RecipeItemName)
			{
				continue;
			}

			FItemData RecipeItemData;
			if (!FindItemData(RecipeItemName, RecipeItemData))
			{
				return;
			}

			UsedInventoryIndices.Add(InventoryIndex);
			ConsumedRecipeValue += RecipeItemData.Price;
			ConsumedRecipeItems.Add(
				{ InventoryIndex, RecipeItemData });
			return;
		}

		if (ActiveRecipePath.Contains(RecipeItemName))
		{
			return;
		}

		FItemData RecipeItemData;
		if (!FindItemData(RecipeItemName, RecipeItemData))
		{
			return;
		}

		ActiveRecipePath.Add(RecipeItemName);
		for (const FName& ChildRecipeItemName :
			RecipeItemData.RecipeItems)
		{
			CollectOwnedRecipeItems(ChildRecipeItemName);
		}
		ActiveRecipePath.Remove(RecipeItemName);
	};

	for (const FName& RecipeItemName : ItemData.RecipeItems)
	{
		CollectOwnedRecipeItems(RecipeItemName);
	}

	constexpr int32 MaxPurchasableItemSlots = 6;
	const int32 InventoryCountAfterPurchase =
		PurchasedItemNames.Num() - ConsumedRecipeItems.Num() + 1;
	if (InventoryCountAfterPurchase > MaxPurchasableItemSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("Buy item failed. Inventory is full. Item=%s"), *ItemName.ToString());
		return;
	}

	const int32 PurchasePrice =
		FMath::Max(0, ItemData.Price - ConsumedRecipeValue);
	if (!Champion->StatComponent->SpendGold(
		static_cast<float>(PurchasePrice)))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Buy item failed. Not enough gold. Item=%s Price=%d Gold=%.0f"),
			*ItemName.ToString(),
			PurchasePrice,
			Champion->StatComponent->GetCurrentGold()
		);
		return;
	}

	const FChampionStat BeforeStat = Champion->StatComponent->GetStat();
	ConsumedRecipeItems.Sort(
		[](const FConsumedRecipeItem& Left,
			const FConsumedRecipeItem& Right)
		{
			return Left.InventoryIndex > Right.InventoryIndex;
		});

	for (const FConsumedRecipeItem& ConsumedItem :
		ConsumedRecipeItems)
	{
		Champion->StatComponent->RemoveItemData(
			ConsumedItem.ItemData);
		PurchasedItemNames.RemoveAt(
			ConsumedItem.InventoryIndex);
	}

	Champion->StatComponent->ApplyItemData(ItemData);
	const FChampionStat AfterStat = Champion->StatComponent->GetStat();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Buy item succeeded. Item=%s Price=%d HP %.0f->%.0f MP %.0f->%.0f AD %.1f->%.1f AP %.1f->%.1f AS %.3f->%.3f Armor %.1f->%.1f MR %.1f->%.1f MS %.1f->%.1f"),
		*ItemName.ToString(),
		PurchasePrice,
		BeforeStat.MaxHP,
		AfterStat.MaxHP,
		BeforeStat.MaxMP,
		AfterStat.MaxMP,
		BeforeStat.AttackDamage,
		AfterStat.AttackDamage,
		BeforeStat.AbilityPower,
		AfterStat.AbilityPower,
		BeforeStat.AttackSpeed,
		AfterStat.AttackSpeed,
		BeforeStat.Armor,
		AfterStat.Armor,
		BeforeStat.SpellBlock,
		AfterStat.SpellBlock,
		BeforeStat.MoveSpeed,
		AfterStat.MoveSpeed
	);

	const FName PurchasedItemName = ItemData.Name.IsNone() ? ItemName : ItemData.Name;
	PurchasedItemNames.Add(PurchasedItemName);
	Client_OnInventoryChanged(PurchasedItemNames);
}

void ALOL_PlayerController::Server_SellItem_Implementation(int32 ItemSlotIndex)
{
	ABaseChampion* Champion = MyChampion ? MyChampion : Cast<ABaseChampion>(GetPawn());
	if (!Champion || !Champion->StatComponent)
	{
		return;
	}

	if (!PurchasedItemNames.IsValidIndex(ItemSlotIndex))
	{
		return;
	}

	const FName ItemName = PurchasedItemNames[ItemSlotIndex];

	if (!IsInShopRange())
	{
		UE_LOG(LogTemp, Warning, TEXT("Sell item failed. Champion is not in shop range. Item=%s"), *ItemName.ToString());
		return;
	}

	FItemData ItemData;
	if (!FindItemData(ItemName, ItemData))
	{
		UE_LOG(LogTemp, Warning, TEXT("Sell item failed. Item row not found. Item=%s"), *ItemName.ToString());
		return;
	}

	const FName SoldItemName = ItemData.Name.IsNone() ? ItemName : ItemData.Name;

	const FChampionStat BeforeStat = Champion->StatComponent->GetStat();
	Champion->StatComponent->RemoveItemData(ItemData);
	Champion->StatComponent->AddGold(
		static_cast<float>(ItemData.Price));
	PurchasedItemNames.RemoveAt(ItemSlotIndex);
	const FChampionStat AfterStat = Champion->StatComponent->GetStat();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Sell item succeeded. Item=%s Refund=%d HP %.0f->%.0f MP %.0f->%.0f AD %.1f->%.1f AP %.1f->%.1f AS %.3f->%.3f Armor %.1f->%.1f MR %.1f->%.1f MS %.1f->%.1f"),
		*SoldItemName.ToString(),
		ItemData.Price,
		BeforeStat.MaxHP,
		AfterStat.MaxHP,
		BeforeStat.MaxMP,
		AfterStat.MaxMP,
		BeforeStat.AttackDamage,
		AfterStat.AttackDamage,
		BeforeStat.AbilityPower,
		AfterStat.AbilityPower,
		BeforeStat.AttackSpeed,
		AfterStat.AttackSpeed,
		BeforeStat.Armor,
		AfterStat.Armor,
		BeforeStat.SpellBlock,
		AfterStat.SpellBlock,
		BeforeStat.MoveSpeed,
		AfterStat.MoveSpeed
	);

	Client_OnInventoryChanged(PurchasedItemNames);
}

void ALOL_PlayerController::UpdateCursorSelection()
{
	if (!IsLocalController() || !MyCursorWidget) return;
	if (!MyChampion) return;

	FHitResult Hit;
	if (GetTargetAwareHitUnderCursor(Hit))
	{
		AActor* TargetActor = Hit.GetActor();
		bool bIsEnemyTarget = false;
		if (TargetActor && TargetActor != MyChampion && TargetActor->FindComponentByClass<ULOL_StateComponent>())
		{
			bIsEnemyTarget = MyChampion->IsEnemyActor(TargetActor);
		}
		if (bIsEnemyTarget && MyChampion->GetIsPressA())
		{
			ChangeCursorType(TEXT("SelectEnemy"));
			return;
		}
		else if (bIsEnemyTarget)
		{
			ChangeCursorType(TEXT("Attack")); 
			return;
		}
		else if (MyChampion->GetIsPressA())
		{
			ChangeCursorType(TEXT("Select")); 
			return;
		}
	}

	ChangeCursorType(TEXT("Normal")); 
}

bool ALOL_PlayerController::GetTargetAwareHitUnderCursor(FHitResult& OutHit) const
{
	FHitResult VisibilityHit;
	const bool bHasVisibilityHit =
		GetHitResultUnderCursor(ECC_Visibility, false, VisibilityHit);

	if (bHasVisibilityHit)
	{
		OutHit = VisibilityHit;
		if (IsClickableAttackTarget(VisibilityHit.GetActor()))
		{
			return true;
		}
	}

	if (!MyChampion)
	{
		return bHasVisibilityHit;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return bHasVisibilityHit;
	}

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CursorExpandedTargetTrace), false);
	QueryParams.AddIgnoredActor(MyChampion);

	const FVector TraceEnd =
		WorldOrigin + WorldDirection.GetSafeNormal() * ExpandedTargetTraceDistance;

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		WorldOrigin,
		TraceEnd,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ExpandedTargetTraceRadius),
		QueryParams
	);

	if (!bHit)
	{
		return bHasVisibilityHit;
	}

	Hits.Sort([](const FHitResult& A, const FHitResult& B)
	{
		return A.Distance < B.Distance;
	});

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsExpandedClickableAttackTarget(HitActor))
		{
			continue;
		}

		OutHit = Hit;
		OutHit.Location = HitActor->GetActorLocation();
		OutHit.ImpactPoint = HitActor->GetActorLocation();
		return true;
	}

	return bHasVisibilityHit;
}

bool ALOL_PlayerController::IsClickableAttackTarget(AActor* TargetActor) const
{
	return
		MyChampion &&
		TargetActor &&
		TargetActor != MyChampion &&
		TargetActor->FindComponentByClass<ULOL_StateComponent>() &&
		MyChampion->IsEnemyActor(TargetActor);
}

bool ALOL_PlayerController::IsExpandedClickableAttackTarget(AActor* TargetActor) const
{
	if (!IsClickableAttackTarget(TargetActor))
	{
		return false;
	}

	return Cast<ABaseMinion>(TargetActor) || Cast<ABaseJungleMonster>(TargetActor);
}

bool ALOL_PlayerController::IsInShopRange() const
{
	if (!bRequireShopRange || ShopPurchaseRange <= 0.f)
	{
		return true;
	}

	return IsNearTeamShop();
}

bool ALOL_PlayerController::IsNearTeamShop() const
{
	if (ShopPurchaseRange <= 0.f)
	{
		return false;
	}

	const ABaseChampion* Champion = MyChampion ? MyChampion : Cast<ABaseChampion>(GetPawn());
	if (!Champion)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector ChampionLocation = Champion->GetActorLocation();
	const bool bChampionIsBlueTeam =
		Champion->StateComponent &&
		Champion->StateComponent->HasStatusTag(LOLTags::Team_Blue);
	const bool bChampionIsRedTeam =
		Champion->StateComponent &&
		Champion->StateComponent->HasStatusTag(LOLTags::Team_Red);

	for (const FName& Tag : ShopActorTags)
	{
		if (Tag.IsNone())
		{
			continue;
		}

		TArray<AActor*> ShopActors;
		UGameplayStatics::GetAllActorsWithTag(World, Tag, ShopActors);

		for (AActor* ShopActor : ShopActors)
		{
			if (!ShopActor)
			{
				continue;
			}

			if (bChampionIsBlueTeam && !ShopActor->ActorHasTag(FName("BlueTeam")))
			{
				continue;
			}
			if (bChampionIsRedTeam && !ShopActor->ActorHasTag(FName("RedTeam")))
			{
				continue;
			}

			if (FVector::Dist2D(ChampionLocation, ShopActor->GetActorLocation()) <= ShopPurchaseRange)
			{
				return true;
			}
		}
	}

	return false;
}

bool ALOL_PlayerController::FindItemData(FName ItemName, FItemData& OutItemData) const
{
	if (!ItemDataTable || ItemName.IsNone())
	{
		return false;
	}

	if (FItemData* FoundRow = ItemDataTable->FindRow<FItemData>(ItemName, TEXT("BuyItem")))
	{
		OutItemData = *FoundRow;
		return true;
	}

	TArray<FItemData*> Rows;
	ItemDataTable->GetAllRows<FItemData>(TEXT("BuyItemFallback"), Rows);
	for (const FItemData* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		if (Row->Name == ItemName || FName(*Row->DisplayName) == ItemName)
		{
			OutItemData = *Row;
			return true;
		}
	}

	return false;
}

UTexture2D* ALOL_PlayerController::LoadItemIconTexture(FName ItemName) const
{
	static const TMap<FName, FString> ItemIconIds =
	{
		{ FName("Boots"), TEXT("1001") },
		{ FName("GiantsBelt"), TEXT("1011") },
		{ FName("BlastingWand"), TEXT("1026") },
		{ FName("BFSword"), TEXT("1038") },
		{ FName("NegatronCloak"), TEXT("1057") },
		{ FName("Pickaxe"), TEXT("1037") },
		{ FName("NeedlesslyLargeRod"), TEXT("1058") },
		{ FName("VampiricScepter"), TEXT("1053") },
		{ FName("Dagger"), TEXT("1042") },
		{ FName("LongSword"), TEXT("1036") },
		{ FName("AmplifyingTome"), TEXT("1052") },
		{ FName("BerserkersGreaves"), TEXT("3006") },
		{ FName("BootsOfSwiftness"), TEXT("3009") },
		{ FName("SorcerersShoes"), TEXT("3020") },
		{ FName("PlatedSteelcaps"), TEXT("3047") },
		{ FName("MercurysTreads"), TEXT("3111") },
		{ FName("Kindlegem"), TEXT("3067") },
		{ FName("Sheen"), TEXT("3057") },
		{ FName("ClothArmor"), TEXT("1029") },
		{ FName("NullMagicMantle"), TEXT("1033") },
		{ FName("Zeal"), TEXT("3086") },
		{ FName("ForbiddenIdol"), TEXT("3114") },
		{ FName("GlowingMote"), TEXT("2022") },
		{ FName("RubyCrystal"), TEXT("1028") },
		{ FName("LocketOfTheIronSolari"), TEXT("3190") },
		{ FName("KnightsVow"), TEXT("3109") },
		{ FName("ZekesConvergence"), TEXT("3050") },
		{ FName("Thornmail"), TEXT("3075") },
		{ FName("BlackCleaver"), TEXT("3071") },
		{ FName("SteraksGage"), TEXT("3053") },
		{ FName("MikaelsBlessing"), TEXT("3222") },
		{ FName("Stridebreaker"), TEXT("3077") },
		{ FName("ForceOfNature"), TEXT("3065") },
		{ FName("JakShoTheProtean"), TEXT("3193") },
		{ FName("IcebornGauntlet"), TEXT("3024") },
		{ FName("VoidStaff"), TEXT("3135") },
		{ FName("RabadonsDeathcap"), TEXT("3089") },
		{ FName("Stormsurge"), TEXT("3100") },
		{ FName("Shadowflame"), TEXT("3135") },
		{ FName("CosmicDrive"), TEXT("3116") },
		{ FName("LordDominiksRegards"), TEXT("3036") },
		{ FName("InfinityEdge"), TEXT("3031") },
		{ FName("GuinsoosRageblade"), TEXT("3124") },
		{ FName("KrakenSlayer"), TEXT("3085") },
		{ FName("TrinityForce"), TEXT("3078") },
		{ FName("SunderedSky"), TEXT("3078") },
		{ FName("Bloodthirster"), TEXT("3072") }
	};

	const FString* IconId = ItemIconIds.Find(ItemName);
	if (!IconId)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item icon id is not mapped. Item=%s"), *ItemName.ToString());
		return nullptr;
	}

	const FString IconPath = FString::Printf(TEXT("/Game/LOL_Data/Img_Item/%s.%s"), **IconId, **IconId);
	UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *IconPath);
	if (!IconTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item icon texture not found. Item=%s Path=%s"), *ItemName.ToString(), *IconPath);
	}

	return IconTexture;
}

void ALOL_PlayerController::Client_OnItemPurchased_Implementation(FName ItemName)
{
	UTexture2D* IconTexture = LoadItemIconTexture(ItemName);
	if (!IconTexture)
	{
		return;
	}

	if (ALOL_HUD* LOLHUD = Cast<ALOL_HUD>(GetHUD()))
	{
		LOLHUD->AddItemIcon(IconTexture);
		UE_LOG(LogTemp, Log, TEXT("Shop item icon added to HUD. Item=%s"), *ItemName.ToString());
	}
}

void ALOL_PlayerController::Client_OnInventoryChanged_Implementation(const TArray<FName>& ItemNames)
{
	PurchasedItemNames = ItemNames;
	SelectedShopItemName = NAME_None;
	SelectedInventoryItemSlotIndex = INDEX_NONE;

	TArray<UTexture2D*> IconTextures;
	for (const FName& ItemName : ItemNames)
	{
		if (UTexture2D* IconTexture = LoadItemIconTexture(ItemName))
		{
			IconTextures.Add(IconTexture);
		}
	}

	if (ALOL_HUD* LOLHUD = Cast<ALOL_HUD>(GetHUD()))
	{
		LOLHUD->SetItemIcons(IconTextures);
		UE_LOG(LogTemp, Log, TEXT("Shop inventory icons refreshed. ItemCount=%d"), ItemNames.Num());
	}
}

void ALOL_PlayerController::BindShopWidgetButtons()
{
	if (bShopButtonsBound || !ShopWidget || !ShopWidget->WidgetTree)
	{
		return;
	}

	TArray<FName> ItemNames;
	CollectShopItemNames(ItemNames);
	if (ItemNames.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop button binding failed. No item rows found in ItemDataTable."));
		return;
	}

	TArray<UButton*> Buttons;
	ShopWidget->WidgetTree->ForEachWidget([&Buttons](UWidget* Widget)
	{
		if (UButton* Button = Cast<UButton>(Widget))
		{
			Buttons.Add(Button);
		}
	});

	if (Buttons.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop button binding failed. No buttons found in shop widget."));
		return;
	}

	UButton* BuyButton = nullptr;
	UButton* SellButton = nullptr;
	for (UButton* Button : Buttons)
	{
		if (DoesButtonLookLikeBuyButton(Button))
		{
			BuyButton = Button;
			continue;
		}

		if (DoesButtonLookLikeSellButton(Button))
		{
			SellButton = Button;
		}
	}

	if (!SellButton && Buttons.Num() > ItemNames.Num() + 1)
	{
		SellButton = Buttons.Last();
	}

	if (!BuyButton && Buttons.Num() > ItemNames.Num())
	{
		for (int32 ButtonIndex = Buttons.Num() - 1; ButtonIndex >= 0; --ButtonIndex)
		{
			if (Buttons[ButtonIndex] != SellButton)
			{
				BuyButton = Buttons[ButtonIndex];
				break;
			}
		}
	}

	int32 ItemIndex = 0;
	for (UButton* Button : Buttons)
	{
		if (!Button || Button == BuyButton || Button == SellButton)
		{
			continue;
		}

		if (!ItemNames.IsValidIndex(ItemIndex))
		{
			break;
		}

		ULOL_ShopButtonBinding* Binding = NewObject<ULOL_ShopButtonBinding>(this);
		Binding->Initialize(this, ItemNames[ItemIndex], false);
		Button->OnClicked.AddDynamic(Binding, &ULOL_ShopButtonBinding::HandleClicked);
		ShopButtonBindings.Add(Binding);
		++ItemIndex;
	}

	if (BuyButton)
	{
		ULOL_ShopButtonBinding* BuyBinding = NewObject<ULOL_ShopButtonBinding>(this);
		BuyBinding->Initialize(this, NAME_None, true, false);
		BuyButton->OnClicked.AddDynamic(BuyBinding, &ULOL_ShopButtonBinding::HandleClicked);
		ShopButtonBindings.Add(BuyBinding);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop buy button was not found. Add a button with text containing '구매' or make it the last button."));
	}

	if (SellButton)
	{
		ULOL_ShopButtonBinding* SellBinding = NewObject<ULOL_ShopButtonBinding>(this);
		SellBinding->Initialize(this, NAME_None, false, true);
		SellButton->OnClicked.AddDynamic(SellBinding, &ULOL_ShopButtonBinding::HandleClicked);
		ShopButtonBindings.Add(SellBinding);
	}

	bShopButtonsBound = true;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Shop buttons bound. ItemButtons=%d BuyButton=%s SellButton=%s"),
		ItemIndex,
		BuyButton ? *BuyButton->GetName() : TEXT("None"),
		SellButton ? *SellButton->GetName() : TEXT("None")
	);
}

bool ALOL_PlayerController::DoesButtonLookLikeBuyButton(UButton* Button) const
{
	if (!Button)
	{
		return false;
	}

	if (Button->GetName().Contains(TEXT("Buy")) ||
		Button->GetName().Contains(TEXT("buy")) ||
		Button->GetName().Contains(TEXT("Purchase")) ||
		Button->GetName().Contains(TEXT("purchase")))
	{
		return true;
	}

	UWidget* Content = Button->GetContent();
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Content))
	{
		const FString Text = TextBlock->GetText().ToString();
		return Text.Contains(TEXT("구매")) ||
			Text.Contains(TEXT("Buy")) ||
			Text.Contains(TEXT("buy")) ||
			Text.Contains(TEXT("Purchase")) ||
			Text.Contains(TEXT("purchase"));
	}

	return false;
}

bool ALOL_PlayerController::DoesButtonLookLikeSellButton(UButton* Button) const
{
	if (!Button)
	{
		return false;
	}

	if (Button->GetName().Contains(TEXT("Sell")) ||
		Button->GetName().Contains(TEXT("sell")) ||
		Button->GetName().Contains(TEXT("Sale")) ||
		Button->GetName().Contains(TEXT("sale")))
	{
		return true;
	}

	UWidget* Content = Button->GetContent();
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Content))
	{
		const FString Text = TextBlock->GetText().ToString();
		return Text.Contains(TEXT("판매")) ||
			Text.Contains(TEXT("Sell")) ||
			Text.Contains(TEXT("sell")) ||
			Text.Contains(TEXT("Sale")) ||
			Text.Contains(TEXT("sale"));
	}

	return false;
}

void ALOL_PlayerController::CollectShopItemNames(TArray<FName>& OutItemNames) const
{
	OutItemNames.Reset();

	if (!ItemDataTable)
	{
		return;
	}

	TArray<FItemData*> Rows;
	ItemDataTable->GetAllRows<FItemData>(TEXT("ShopButtonBinding"), Rows);
	for (const FItemData* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		OutItemNames.Add(Row->Name.IsNone() ? FName(*Row->DisplayName) : Row->Name);
	}
}

void ALOL_PlayerController::ChangeCursorType(FString NewStateName)
{
	if (MyCursorWidget && LastCursorState != NewStateName)
	{
		MyCursorWidget->SwitchCursorState(NewStateName);
		LastCursorState = NewStateName;
	}
}
