// 마우스 커서
#include "Widget/LOL_CursorWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/Image.h"

ULOL_CursorWidget::ULOL_CursorWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 생성자에서 모든 커서 이미지를 하드코딩으로 로드합니다.
    static ConstructorHelpers::FObjectFinder<UTexture2D> NormalTex(TEXT("/Game/UI/Cursor/cursor_Sprite_5.cursor_Sprite_5"));
    if (NormalTex.Succeeded()) CursorTable.Add(TEXT("Normal"), NormalTex.Object);

    static ConstructorHelpers::FObjectFinder<UTexture2D> AttackTex(TEXT("/Game/UI/Cursor/cursor_Sprite_7.cursor_Sprite_7"));
    if (AttackTex.Succeeded()) CursorTable.Add(TEXT("Attack"), AttackTex.Object);

    // 추가하고 싶은 커서가 있다면 계속 여기에 추가...
}
void ULOL_CursorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 이제 CursorImage가 유효한 상태이므로 기본값을 설정할 수 있습니다.
    if (CursorTable.Contains(TEXT("Normal")))
    {
        SwitchCursorState(TEXT("Normal"));
    }
}
void ULOL_CursorWidget::SwitchCursorState(FString StateName)
{
    if (CursorTable.Contains(StateName) && CursorImage)
    {
        CursorImage->SetBrushFromTexture(CursorTable[StateName]);
    }
}