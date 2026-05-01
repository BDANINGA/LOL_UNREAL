// HP. MP 위젯 관련 컴포넌트

#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StatComponent.h"

#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"

#include "BaseChampion.h"

#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ULOL_UIComponent::ULOL_UIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

    HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
    HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
    HpBar->SetWidgetSpace(EWidgetSpace::Screen);
    HpBar->SetDrawSize(FVector2D(150.0f, 20.0f));
    HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/UI/HPbar.HPbar_C"));
    if (HpBarWidgetRef.Succeeded())
    {
        HpBar->SetWidgetClass(HpBarWidgetRef.Class);
    }

    MpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("MpBar"));
    MpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 275.0f));
    MpBar->SetWidgetSpace(EWidgetSpace::Screen);
    MpBar->SetDrawSize(FVector2D(150.0f, 10.0f));
    MpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FClassFinder<UUserWidget> MpBarWidgetRef(TEXT("/Game/UI/MPbar.MPbar_C"));
    if (MpBarWidgetRef.Succeeded())
    {
        MpBar->SetWidgetClass(MpBarWidgetRef.Class);
    }
}

void ULOL_UIComponent::BeginPlay()
{
	Super::BeginPlay();

    if (ABaseChampion* OwnerChampion = Cast<ABaseChampion>(GetOwner()))
    {
        HpBar->AttachToComponent(OwnerChampion->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        MpBar->AttachToComponent(OwnerChampion->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

        if (OwnerChampion->StatComponent)
        {
            CachedMaxHP = OwnerChampion->StatComponent->GetStat().MaxHP; 
            UpdateHpFromStat(OwnerChampion->StatComponent->GetStat().CurrentHP); 
        }
    }
}
void ULOL_UIComponent::UpdateHpFromStat(float NewHp)
{
    if (CachedMaxHP <= 0.f) return;

    if (UUserWidget* WidgetObj = HpBar->GetUserWidgetObject())
    {
        if (UProgressBar* HpProgressBar = Cast<UProgressBar>(WidgetObj->GetWidgetFromName(TEXT("HPbar"))))
        {
            HpProgressBar->SetPercent(NewHp / CachedMaxHP);
        }
    }
}
