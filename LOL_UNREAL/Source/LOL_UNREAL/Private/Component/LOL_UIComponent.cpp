// HP. MP 위젯 관련 컴포넌트

#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/WidgetComponent.h"

#include "Widget/LOL_ChampionWidget.h"
#include "LOL_HUD.h"

#include "BaseChampion.h"
#include "UObject/ConstructorHelpers.h"

ULOL_UIComponent::ULOL_UIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

    ChampionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChampionWidget"));
    ChampionWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f)); // 적절한 높이 조절
    ChampionWidget->SetWidgetSpace(EWidgetSpace::Screen);
    ChampionWidget->SetDrawSize(FVector2D(10.f, 5.f));
    static ConstructorHelpers::FClassFinder<UUserWidget> ChampionWidgetRef(TEXT("/Game/UI/ChampionWidget/Wbp_healthbar.Wbp_healthbar_C"));
    if (ChampionWidgetRef.Succeeded())
    {
        ChampionWidget->SetWidgetClass(ChampionWidgetRef.Class);
    }
}

void ULOL_UIComponent::BeginPlay()
{
    Super::BeginPlay();

    Owner = Cast<ABaseChampion>(GetOwner());
    if (Owner && Owner->StatComponent)
    {
        ChampionWidget->AttachToComponent(Owner->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
    }
}
void ULOL_UIComponent::UpdateHpFromStat(float NewHp)
{
    if (CachedMaxHP <= 0.f) return;

    if (ULOL_ChampionWidget* ChampWidgetObj = Cast<ULOL_ChampionWidget>(ChampionWidget->GetUserWidgetObject()))
    {
        ChampWidgetObj->UpdateHP(NewHp / Owner->StatComponent->GetStat().MaxHP);
    }
}

void ULOL_UIComponent::UpdateMpFromStat(float NewMp)
{
    if (CachedMaxMP <= 0.f) return;

    if (ULOL_ChampionWidget* ChampWidgetObj = Cast<ULOL_ChampionWidget>(ChampionWidget->GetUserWidgetObject()))
    {
        ChampWidgetObj->UpdateMP(NewMp / Owner->StatComponent->GetStat().MaxMP);
    }
}