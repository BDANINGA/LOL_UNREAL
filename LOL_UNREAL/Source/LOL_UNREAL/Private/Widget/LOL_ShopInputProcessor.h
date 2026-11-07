#pragma once

#include "LOL_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"

// Handles shortcuts before SButton consumes left clicks or right clicks reach gameplay.
class FLOL_ShopInputProcessor final : public IInputProcessor
{
public:
	FLOL_ShopInputProcessor(ALOL_PlayerController* InController, UUserWidget* InShop)
		: Controller(InController), Shop(InShop) {}

	void AddItemButton(UButton* Button, FName ItemName)
	{
		ItemButtons.Add(Button, ItemName);
	}

	void ResetInventorySlots()
	{
		InventorySlots.Reset();
	}

	void AddInventorySlot(UWidget* Widget, int32 SlotIndex)
	{
		if (Widget)
		{
			InventorySlots.Add(Widget, SlotIndex);
		}
	}

	void ResetInputState()
	{
		PressedLeftButton.Reset();
		LastClickedLeftButton.Reset();
		ConsumedButtons.Reset();
	}

	virtual void Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		return HandlePress(SlateApp, Event, false);
	}

	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		return HandlePress(SlateApp, Event, true);
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& Event) override
	{
		if (ConsumedButtons.Remove(Event.GetEffectingButton()) > 0)
		{
			return true;
		}

		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			UButton* Button = Cast<UButton>(FindShortcutWidget(SlateApp, Event));
			LastClickedLeftButton = Button == PressedLeftButton.Get() ? Button : nullptr;
			PressedLeftButton.Reset();
		}
		return false;
	}

private:
	UWidget* FindShortcutWidget(FSlateApplication& SlateApp, const FPointerEvent& Event) const
	{
		if (!Controller.IsValid() || !Shop.IsValid() || !Shop->IsInViewport() || Event.IsTouchEvent())
		{
			return nullptr;
		}

		// Use the actual hit path so clipping, overlapping dialogs and other PIE windows are respected.
		const FWidgetPath Path = SlateApp.LocateWindowUnderMouse(
			Event.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows(), false, Event.GetUserIndex());
		for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
		{
			if (!Path.Widgets[Index].Widget->IsEnabled())
			{
				return nullptr;
			}
		}
		for (const auto& Entry : InventorySlots)
		{
			UWidget* Slot = Entry.Key.Get();
			if (Slot && Slot->GetIsEnabled())
			{
				const TSharedPtr<SWidget> Widget = Slot->GetCachedWidget();
				if (Widget.IsValid() && Path.ContainsWidget(Widget.Get()))
				{
					return Slot;
				}
			}
		}
		for (const auto& Entry : ItemButtons)
		{
			UButton* Button = Entry.Key.Get();
			if (Button && Button->GetIsEnabled())
			{
				const TSharedPtr<SWidget> Widget = Button->GetCachedWidget();
				if (Widget.IsValid() && Path.ContainsWidget(Widget.Get()))
				{
					return Button;
				}
			}
		}
		return nullptr;
	}

	bool HandlePress(FSlateApplication& SlateApp, const FPointerEvent& Event, bool bDoubleClick)
	{
		const FKey Key = Event.GetEffectingButton();
		UWidget* HitWidget = FindShortcutWidget(SlateApp, Event);
		if (Key == EKeys::RightMouseButton)
		{
			if (const int32* SlotIndex = InventorySlots.Find(HitWidget))
			{
				LastClickedLeftButton.Reset();
				PressedLeftButton.Reset();
				// Consume both edges so selling never also issues a move command.
				ConsumedButtons.Add(Key);
				Controller->SelectInventoryItem(*SlotIndex);
				Controller->SellSelectedShopItem();
				return true;
			}
		}
		UButton* Button = Cast<UButton>(HitWidget);
		const bool bBuy = Button && ItemButtons.Contains(Button) && (Key == EKeys::RightMouseButton ||
			(Key == EKeys::LeftMouseButton && bDoubleClick && LastClickedLeftButton.Get() == Button));
		LastClickedLeftButton.Reset();
		PressedLeftButton = Key == EKeys::LeftMouseButton && !bBuy ? Button : nullptr;
		if (!bBuy)
		{
			return false;
		}

		// Swallow the matching release too: one gesture must issue only one purchase.
		ConsumedButtons.Add(Key);
		Controller->SelectShopItem(ItemButtons.FindChecked(Button));
		Controller->BuySelectedShopItem();
		return true;
	}

	TWeakObjectPtr<ALOL_PlayerController> Controller;
	TWeakObjectPtr<UUserWidget> Shop;
	TMap<TWeakObjectPtr<UButton>, FName> ItemButtons;
	TMap<TWeakObjectPtr<UWidget>, int32> InventorySlots;
	TWeakObjectPtr<UButton> PressedLeftButton;
	TWeakObjectPtr<UButton> LastClickedLeftButton;
	TSet<FKey> ConsumedButtons;
};
