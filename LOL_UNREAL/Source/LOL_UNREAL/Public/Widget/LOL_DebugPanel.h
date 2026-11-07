#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LOL_DebugPanel.generated.h"

UCLASS()
class ULOL_DebugButton : public UButton
{
    GENERATED_BODY()
public:
    ULOL_DebugButton(const FObjectInitializer& Init) : Super(Init) { InitIsFocusable(false); }
};

UCLASS()
class LOL_UNREAL_API ULOL_DebugPanel : public UUserWidget
{
    GENERATED_BODY()
public:
    void Refresh();
protected:
    virtual void NativeOnInitialized() override;
private:
    void Dispatch(int32 Row);
    UFUNCTION() void Recover() { Dispatch(1); }
    UFUNCTION() void LevelUp() { Dispatch(2); }
    UFUNCTION() void Gold() { Dispatch(3); }
    UFUNCTION() void Teleport() { Dispatch(4); }
    UFUNCTION() void Cooldown() { Dispatch(5); }
    UFUNCTION() void Home() { Dispatch(6); }
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> RecoverState;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> CooldownState;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Hint;
};
