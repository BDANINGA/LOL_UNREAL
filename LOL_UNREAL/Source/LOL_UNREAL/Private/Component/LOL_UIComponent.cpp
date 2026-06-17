// HP. MP 위젯 관련 컴포넌트

#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StatComponent.h"

#include "Components/WidgetComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Widget/LOL_ChampionWidget.h"
#include "Widget/LOL_MinionWidget.h"
#include "LOL_HUD.h"
#include "BaseChampion.h"
#include "JungleMonster/BaseJungleMonster.h"

ULOL_UIComponent::ULOL_UIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

    ActorWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ActorWidget"));
    ActorWidget->SetWidgetSpace(EWidgetSpace::Screen);

    static ConstructorHelpers::FClassFinder<UUserWidget> ChampRef(TEXT("/Game/UI/ChampionWidget/Wbp_healthbar.Wbp_healthbar_C"));
    if (ChampRef.Succeeded()) ChampionWidgetClass = ChampRef.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> MinionRef(TEXT("/Game/UI/minion_widget/WBP_MinionWidget.WBP_MinionWidget_C"));
    if (MinionRef.Succeeded()) MinionWidgetClass = MinionRef.Class;

    static ConstructorHelpers::FClassFinder<UUserWidget> JungleMonsterRef(TEXT("/Game/UI/minion_widget/minion_hp_enemy.minion_hp_enemy_C"));
    if (JungleMonsterRef.Succeeded()) JungleMonsterWidgetClass = JungleMonsterRef.Class;

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

    OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn)
    {
        if (USkeletalMeshComponent* MeshComp = OwnerPawn->FindComponentByClass<USkeletalMeshComponent>())
        {
            ActorWidget->AttachToComponent(MeshComp, FAttachmentTransformRules::KeepRelativeTransform);
        }

        if (ABaseChampion* Champ = Cast<ABaseChampion>(OwnerPawn))
        {
            ActorWidget->SetWidgetClass(ChampionWidgetClass);
            ActorWidget->SetDrawSize(FVector2D(200.f, 20.f));
            ActorWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));

            RangeIndicator->AttachToComponent(Champ->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
        else if (Cast<ABaseJungleMonster>(OwnerPawn))
        {
            ActorWidget->SetWidgetClass(JungleMonsterWidgetClass ? JungleMonsterWidgetClass : MinionWidgetClass);
            ActorWidget->SetDrawSize(FVector2D(50.f, 5.f));
            ActorWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));

            RangeIndicator->DestroyComponent();
        }
        else
        {
            ActorWidget->SetWidgetClass(MinionWidgetClass);
            ActorWidget->SetDrawSize(FVector2D(50.f, 5.f));
            ActorWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
            
            RangeIndicator->DestroyComponent();
        }
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
    ABaseChampion* Champ = Cast<ABaseChampion>(OwnerPawn);
    if (!Champ->IsLocallyControlled() || !RangeIndicator) return;

    float Range{};
    if (Champ->StatComponent) Range = Champ->StatComponent->GetStat().AttackRange;

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
    ABaseChampion* Champ = Cast<ABaseChampion>(OwnerPawn);
    if (!Champ->IsLocallyControlled() || !RangeIndicator) return;

    RangeIndicator->SetHiddenInGame(true);
}

void ULOL_UIComponent::UpdateHpFromStat(float NewHp)
{
    if (CachedMaxHP <= 0.f || !OwnerPawn) return;

    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    if (!StatComp) return;

    float Percent = NewHp / StatComp->GetStat().MaxHP;
    UUserWidget* UserWidgetObj = ActorWidget->GetUserWidgetObject();
    if (!UserWidgetObj) return;


    if (ULOL_ChampionWidget* ChampWidgetObj = Cast<ULOL_ChampionWidget>(UserWidgetObj))
    {
        ChampWidgetObj->UpdateHP(Percent);
    }
    else if (ULOL_MinionWidget* MinionWidgetObj = Cast<ULOL_MinionWidget>(UserWidgetObj))
    {
        MinionWidgetObj->UpdateHP(Percent);
    }
}

void ULOL_UIComponent::UpdateMpFromStat(float NewMp)
{
    if (CachedMaxMP <= 0.f || !OwnerPawn) return;
    
    ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();
    if (!StatComp) return;
    
    if (ULOL_ChampionWidget* ChampWidgetObj = Cast<ULOL_ChampionWidget>(ActorWidget->GetUserWidgetObject()))
    {
        ChampWidgetObj->UpdateMP(NewMp / StatComp->GetStat().MaxMP);
    }
}
