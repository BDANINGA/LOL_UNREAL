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

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_1;
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_2;
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_3;
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_4;
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_5;
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemSlot_6;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HPProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* MPProgressBar;

    /*UPROPERTY(meta = (BindWidget))
    class UProgressBar* EXPProgressBar;*/

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

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_HP;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_MP;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Gold;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Level;

    void SetAttackDamage(float Value) { if (Txt_AD) Txt_AD->SetText(FText::AsNumber(Value)); }
    void SetAbilityPower(float Value) { if (Txt_AP) Txt_AP->SetText(FText::AsNumber(Value)); }
    void SetArmor(float Value) { if (Txt_Armor) Txt_Armor->SetText(FText::AsNumber(Value)); }
    void SetSpellBlock(float Value) { if (Txt_MR) Txt_MR->SetText(FText::AsNumber(Value)); }
    void SetAttackSpeed(float Value) { if (Txt_AS) Txt_AS->SetText(FText::AsNumber(Value)); }
    void SetAbilityHaste(float Value) { if (Txt_AH) Txt_AH->SetText(FText::AsNumber(Value)); }
    void SetCriticalRate(float Value) { if (Txt_Crit) Txt_Crit->SetText(FText::AsNumber(Value)); }
    void SetMoveSpeed(float Value) { if (Txt_MS) Txt_MS->SetText(FText::AsNumber(Value)); }
    void SetLevel(float Value) { if (Txt_Level) Txt_Level->SetText(FText::AsNumber(Value)); }

    void UpdateHP(float NewHP, float MaxHP);
    void UpdateMP(float NewMP, float MaxMP);
    void UpdateEXP(float NewEXP, float MaxEXP);
    void UpdateGold(float NewGold) { if (Txt_Gold) Txt_Gold->SetText(FText::AsNumber(NewGold)); }

    void SetSkillCooldown(FName SkillName, float CoolLocalEndTime, float CoolEndTime);
    void SetSkillImage(FName SkillName, UTexture2D* IconTexture);
    void AddItemIcon(UTexture2D* IconTexture);
    void SetItemIcons(const TArray<UTexture2D*>& IconTextures);
protected:
    UPROPERTY()
    class UMaterialInstanceDynamic* SkillQ_MID;
    UPROPERTY()
    class UMaterialInstanceDynamic* SkillW_MID;
    UPROPERTY()
    class UMaterialInstanceDynamic* SkillE_MID;
    UPROPERTY()
    class UMaterialInstanceDynamic* SkillR_MID;

    UPROPERTY()
    float SkillCoolLocalEndTimeQ;
    UPROPERTY()
    float SkillCoolEndTimeQ;
    UPROPERTY()
    float SkillCoolLocalEndTimeW;
    UPROPERTY()
    float SkillCoolEndTimeW;
    UPROPERTY()
    float SkillCoolLocalEndTimeE;
    UPROPERTY()
    float SkillCoolEndTimeE;
    UPROPERTY()
    float SkillCoolLocalEndTimeR;
    UPROPERTY()
    float SkillCoolEndTimeR;
    UPROPERTY()
    float SkillCoolLocalEndTimeP;
    UPROPERTY()
    float SkillCoolEndTimeP;

    UPROPERTY()
    TArray<class UImage*> CachedItemSlotImages;

    UPROPERTY()
    int32 NextItemSlotIndex = 0;

    void CacheItemSlotImages();

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
