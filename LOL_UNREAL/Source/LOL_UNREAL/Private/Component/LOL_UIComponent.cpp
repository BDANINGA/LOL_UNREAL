// HP. MP 위젯 관련 컴포넌트

#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StatComponent.h"

#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

    RangeIndicator = CreateDefaultSubobject<UDecalComponent>(TEXT("RangeIndicator"));
    RangeIndicator->SetRelativeLocation(FVector(0.f, 0.f, -90.f)); 
    RangeIndicator->SetHiddenInGame(true);
    RangeIndicator->bIsEditorOnly = false;
    RangeIndicator->SetComponentTickEnabled(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMatRef(TEXT("/Game/UI/RangeIndicator/circularrangeindicatorul_mat.circularrangeindicatorul_mat"));
    if (DecalMatRef.Succeeded())
    {
        BaseDecalMaterial = DecalMatRef.Object;
    }
}

void ULOL_UIComponent::BeginPlay()
{
    Super::BeginPlay();

    Owner = Cast<ABaseChampion>(GetOwner());
    if (Owner && Owner->StatComponent)
    {
        ChampionWidget->AttachToComponent(Owner->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        RangeIndicator->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    }
    if (BaseDecalMaterial)
    {
        // 원본 머티리얼을 기반으로 복사본(MID)을 만들어 데칼에 씌워줍니다.
        DecalMID = UMaterialInstanceDynamic::Create(BaseDecalMaterial, this);
        RangeIndicator->SetDecalMaterial(DecalMID);
    }
}

void ULOL_UIComponent::ShowRangeIndicator()
{
    if (!Owner->IsLocallyControlled() || !RangeIndicator) return;

    float Range{};
    if (Owner->StatComponent) Range = Owner->StatComponent->GetStat().AttackRange;

    RangeIndicator->DecalSize = FVector(500.f, Range, Range);

    // 머티리얼 노드에서 만들었던 'IndicatorDiameter' 파라미터에 '지름(반지름 * 2)' 값을 전달
    if (DecalMID)
    {
        DecalMID->SetScalarParameterValue(TEXT("IndicatorDiameter"), Range * 2.0f);
    }

    RangeIndicator->SetHiddenInGame(false);
}

void ULOL_UIComponent::HideRangeIndicator()
{
    if (!Owner->IsLocallyControlled() || !RangeIndicator) return;

    RangeIndicator->SetHiddenInGame(true);
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