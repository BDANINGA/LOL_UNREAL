// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// ---------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LOL_PlayerController.generated.h"

class ALOL_PlayerController;

UCLASS()
class LOL_UNREAL_API ULOL_ShopButtonBinding : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ALOL_PlayerController* InOwnerController, FName InItemName, bool bInBuyButton, bool bInSellButton = false);

	UFUNCTION()
	void HandleClicked();

private:
	UPROPERTY()
	ALOL_PlayerController* OwnerController = nullptr;

	UPROPERTY()
	FName ItemName = NAME_None;

	UPROPERTY()
	bool bBuyButton = false;

	UPROPERTY()
	bool bSellButton = false;
};

UCLASS()
class LOL_UNREAL_API ALOL_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALOL_PlayerController();

	void SetSelectedChampionClass(TSubclassOf<class ABaseChampion> InChampionClass);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Data")
	TSubclassOf<class ABaseChampion> SelectedChampionClass;

	void OnRightClick();
	void OnLeftClick();
	void OnSkillQ();
	void OnSkillW();
	void OnSkillE();
	void OnSkillR();
	void OnAKey();
	void OnToggleShop();
	void OnRecall();
	void OnQuitGame();

	void OnToggleCamera();
	void FreeCameraEdgeScroll(float DeltaTime);
	virtual void AcknowledgePossession(APawn* P) override;
	void InitCameraAnchor(APawn* TargetPawn);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class ULOL_CursorWidget> CursorWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> ShopWidgetClass;

	void UpdateCursorSelection();
	void ChangeCursorType(FString NewStateName);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestBuyItem(FName ItemName);

	void SelectShopItem(FName ItemName);
	void SelectInventoryItem(int32 ItemSlotIndex);
	void BuySelectedShopItem();
	void SellSelectedShopItem();
	bool IsNearTeamShop() const;

	UFUNCTION(Server, Reliable)
	void Server_BuyItem(FName ItemName);

	UFUNCTION(Server, Reliable)
	void Server_SellItem(int32 ItemSlotIndex);

	UFUNCTION(Client, Reliable)
	void Client_OnItemPurchased(FName ItemName);

	UFUNCTION(Client, Reliable)
	void Client_OnInventoryChanged(const TArray<FName>& ItemNames);

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;

	virtual void SetupInputComponent() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* RightClickAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LeftClickAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillQAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillWAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillEAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillRAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SpaceBarAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AKeyAction;
	
	UPROPERTY()
	class UNiagaraSystem* ClickFX;
	UPROPERTY()
	class UNiagaraSystem* AClickFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Targeting")
	float ExpandedTargetTraceRadius = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Targeting")
	float ExpandedTargetTraceDistance = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	class UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	float ShopPurchaseRange = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	bool bRequireShopRange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TArray<FName> ShopActorTags;
private:
	bool GetTargetAwareHitUnderCursor(FHitResult& OutHit) const;
	bool IsClickableAttackTarget(AActor* TargetActor) const;
	bool IsExpandedClickableAttackTarget(AActor* TargetActor) const;
	bool IsInShopRange() const;
	bool FindItemData(FName ItemName, struct FItemData& OutItemData) const;
	class UTexture2D* LoadItemIconTexture(FName ItemName) const;
	void BindShopWidgetButtons();
	bool DoesButtonLookLikeBuyButton(class UButton* Button) const;
	bool DoesButtonLookLikeSellButton(class UButton* Button) const;
	void CollectShopItemNames(TArray<FName>& OutItemNames) const;

	UPROPERTY()
	class ACamera* CameraAnchor;

	UPROPERTY()
	class ABaseChampion* MyChampion;

	UPROPERTY()
	class ULOL_CursorWidget* MyCursorWidget;

	UPROPERTY()
	class UUserWidget* ShopWidget;

	UPROPERTY()
	TArray<ULOL_ShopButtonBinding*> ShopButtonBindings;

	UPROPERTY()
	FName SelectedShopItemName = NAME_None;

	UPROPERTY()
	int32 SelectedInventoryItemSlotIndex = INDEX_NONE;

	UPROPERTY()
	TArray<FName> PurchasedItemNames;

	bool bShopButtonsBound = false;

	FString LastCursorState;
};
