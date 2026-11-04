#include "Widget/LOL_DebugPanel.h"
#include "LOL_PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

void ULOL_DebugPanel::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    const FLinearColor Gold(.83f,.64f,.30f);
    auto Label = [&](const TCHAR* Value, int32 Size, FLinearColor Color)
    {
        auto* T = WidgetTree->ConstructWidget<UTextBlock>();
        T->SetText(FText::FromString(Value));
        T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
        T->SetColorAndOpacity(FSlateColor(Color));
        return T;
    };
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = Canvas;
    auto* Width = WidgetTree->ConstructWidget<USizeBox>();
    Width->SetWidthOverride(290.f);
    auto* Placement = Canvas->AddChildToCanvas(Width);
    Placement->SetAnchors(FAnchors(0.f,.5f));
    Placement->SetAlignment(FVector2D(0,.5));
    Placement->SetAutoSize(true);
    Placement->SetPosition(FVector2D(18,0));
    auto* Frame = WidgetTree->ConstructWidget<UBorder>();
    Frame->SetBrushColor(FLinearColor(.42f,.31f,.13f)); Frame->SetPadding(FMargin(1.5f));
    Width->SetContent(Frame);
    auto* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(.012f,.027f,.04f,.97f)); Panel->SetPadding(FMargin(14,12));
    Frame->SetContent(Panel);
    auto* List = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(List);
    List->AddChildToVerticalBox(Label(TEXT("디버그 모드"),19,Gold))->SetPadding(FMargin(2,0,0,7));
    const TCHAR* Names[] = {TEXT("체력 · 마나 자동 회복"),TEXT("레벨업"),TEXT("골드 +5,000"),TEXT("마우스 위치로 순간이동"),TEXT("쿨타임 자동 초기화"),TEXT("기지 복귀")};
    const FName Actions[] = {GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,Recover),GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,LevelUp),GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,Gold),GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,Teleport),GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,Cooldown),GET_FUNCTION_NAME_CHECKED(ULOL_DebugPanel,Home)};
    for (int32 I=0; I<6; ++I)
    {
        auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetHeightOverride(38);
        List->AddChildToVerticalBox(Size)->SetPadding(FMargin(0,2));
        auto* Button = WidgetTree->ConstructWidget<ULOL_DebugButton>(ULOL_DebugButton::StaticClass(),FName(*FString::Printf(TEXT("DebugButton%d"),I+1)));
        Button->SetBackgroundColor(FLinearColor(.14f,.24f,.30f));
        Size->SetContent(Button);
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Button->SetContent(Row);
        CastChecked<UButtonSlot>(Row->Slot)->SetHorizontalAlignment(HAlign_Fill);
        CastChecked<UButtonSlot>(Row->Slot)->SetPadding(FMargin(9,2));
        Row->AddChildToHorizontalBox(Label(*FString::FromInt(I+1),14,Gold))->SetPadding(FMargin(0,0,12,0));
        Row->AddChildToHorizontalBox(Label(Names[I],14,FLinearColor(.91f,.93f,.93f)))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        if (I==0 || I==4)
        {
            auto* State=Label(TEXT("OFF"),12,FLinearColor::Gray);
            Row->AddChildToHorizontalBox(State)->SetVerticalAlignment(VAlign_Center);
            if (I==0) RecoverState=State; else CooldownState=State;
        }
        FScriptDelegate Click; Click.BindUFunction(this,Actions[I]); Button->OnClicked.AddUnique(Click);
    }
    Hint=Label(TEXT(""),12,FLinearColor(.62f,.77f,.79f));
    List->AddChildToVerticalBox(Hint)->SetPadding(FMargin(2,9,0,0));
    Refresh();
}

void ULOL_DebugPanel::Dispatch(int32 Row)
{
    if (auto* PC=GetOwningPlayer<ALOL_PlayerController>()) PC->RunDebugPanelAction(Row);
}

void ULOL_DebugPanel::Refresh()
{
    const auto* PC=GetOwningPlayer<ALOL_PlayerController>();
    if (!PC || !Hint) return;
    auto SetState=[](UTextBlock* Label,bool bOn)
    {
        Label->SetText(FText::FromString(bOn?TEXT("ON"):TEXT("OFF")));
        Label->SetColorAndOpacity(FSlateColor(bOn?FLinearColor(.30f,.98f,.68f):FLinearColor(.63f,.69f,.72f)));
    };
    SetState(RecoverState,PC->bIsAutoRecoverActive);
    SetState(CooldownState,PC->bIsAutoResetCooldownActive);
    Hint->SetText(FText::FromString(PC->bPickingDebugDestination?TEXT("전장에서 목적지를 클릭하세요"):TEXT("1–6  실행     ·     F3  닫기")));
}
