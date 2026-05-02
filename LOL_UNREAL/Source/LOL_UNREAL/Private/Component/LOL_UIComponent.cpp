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

    ChampionProgressBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChampionProgressBar"));
    ChampionProgressBar->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f)); // 적절한 높이 조절
    ChampionProgressBar->SetWidgetSpace(EWidgetSpace::Screen);
    ChampionProgressBar->SetDrawSize(FVector2D(200.f, 50.f));
    ChampionProgressBar->SetPivot(FVector2D(0.5f, 1.0f));
    static ConstructorHelpers::FClassFinder<UUserWidget> ChampionProgressBarWidgetRef(TEXT("/Game/UI/WBP_ChampionProgressBar.WBP_ChampionProgressBar_C"));
    if (ChampionProgressBarWidgetRef.Succeeded())
    {
        ChampionProgressBar->SetWidgetClass(ChampionProgressBarWidgetRef.Class);
    }
}

void ULOL_UIComponent::BeginPlay()
{
	Super::BeginPlay();

    if (ABaseChampion* OwnerChampion = Cast<ABaseChampion>(GetOwner()))
    {
        ChampionProgressBar->AttachToComponent(OwnerChampion->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);

        if (OwnerChampion->StatComponent)
        {
            const FChampionStat& CurrentStat = OwnerChampion->StatComponent->GetStat();

            CachedMaxMP = CurrentStat.MaxMP;
            CachedMaxHP = CurrentStat.MaxHP;

            UpdateHpFromStat(OwnerChampion->StatComponent->GetStat().CurrentHP);
            UpdateMpFromStat(OwnerChampion->StatComponent->GetStat().CurrentMP);
        }
    }
}
void ULOL_UIComponent::UpdateHpFromStat(float NewHp)
{
    if (CachedMaxHP <= 0.f) return;

    if (UUserWidget* WidgetObj = ChampionProgressBar->GetUserWidgetObject())
    {
        if (UProgressBar* HpProgressBar = Cast<UProgressBar>(WidgetObj->GetWidgetFromName(TEXT("HPbar"))))
        {
            HpProgressBar->SetPercent(NewHp / CachedMaxHP);
        }
    }
}

void ULOL_UIComponent::UpdateMpFromStat(float NewMp)
{
    if (CachedMaxMP <= 0.f) return;

    if (UUserWidget* WidgetObj = ChampionProgressBar->GetUserWidgetObject())
    {
        if (UProgressBar* MpProgressBar = Cast<UProgressBar>(WidgetObj->GetWidgetFromName(TEXT("MPbar"))))
        {
            MpProgressBar->SetPercent(NewMp / CachedMaxMP);
        }
    }
}
