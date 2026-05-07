// HUD 관리하는 위젯 클래스
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LOL_HUDWidget.generated.h"

UCLASS()
class LOL_UNREAL_API ULOL_HUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UPROPERTY(meta = (BindWidget))
    class UImage* Portrait_Image;
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillQ_Image;
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillW_Image;
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillE_Image;
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillR_Image;
    UPROPERTY(meta = (BindWidget))
    class UImage* SkillP_Image;


    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AD;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AP;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Armor;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_MR;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AS;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AH;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Crit;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_MS;

    void SetAttackDamage(float Value) { if (Txt_AD) Txt_AD->SetText(FText::AsNumber(Value)); }
    void SetAbilityPower(float Value) { if (Txt_AP) Txt_AP->SetText(FText::AsNumber(Value)); }
    void SetArmor(float Value) { if (Txt_Armor) Txt_Armor->SetText(FText::AsNumber(Value)); }
    void SetSpellBlock(float Value) { if (Txt_MR) Txt_MR->SetText(FText::AsNumber(Value)); }
    void SetAttackSpeed(float Value) { if (Txt_AS) Txt_AS->SetText(FText::AsNumber(Value)); }
    void SetAbilityHaste(float Value) { if (Txt_AH) Txt_AH->SetText(FText::AsNumber(Value)); }
    void SetCriticalRate(float Value) { if (Txt_Crit) Txt_Crit->SetText(FText::AsNumber(Value)); }
    void SetMoveSpeed(float Value) { if (Txt_MS) Txt_MS->SetText(FText::AsNumber(Value)); }
};
