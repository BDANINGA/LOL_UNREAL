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
        else if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn))
        {
            ActorWidget->SetWidgetClass(JungleMonsterWidgetClass ? JungleMonsterWidgetClass : MinionWidgetClass);
            ActorWidget->InitWidget();
            if (ULOL_MinionWidget* MinionWidgetObj = Cast<ULOL_MinionWidget>(ActorWidget->GetUserWidgetObject()))
            {
                MinionWidgetObj->SetHPBarColor(FLinearColor::Red);
            }
            ApplyJungleMonsterWidgetLayout(JungleMonster);

            RangeIndicator->DestroyComponent();
        }
        else
        {
            ActorWidget->SetWidgetClass(MinionWidgetClass);

            ActorWidget->InitWidget();

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

void ULOL_UIComponent::ApplyJungleMonsterWidgetLayout(ABaseJungleMonster* JungleMonster)
{
    if (!ActorWidget || !JungleMonster)
    {
        return;
    }

    FVector2D DrawSize(200.0f, 10.0f);
    FVector RelativeLocation(0.0f, 0.0f, 200.0f);

    const FName MonsterName = JungleMonster->GetJungleMonsterName();

    if (MonsterName == FName("Wolf"))
    {
        DrawSize = FVector2D(120.0f, 8.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 160.0f);
    }
    else if (MonsterName == FName("Gromp"))
    {
        DrawSize = FVector2D(140.0f, 8.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 180.0f);
    }
    else if (MonsterName == FName("Razorbeak") || MonsterName == FName("Raptor"))
    {
        DrawSize = FVector2D(110.0f, 7.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 150.0f);
    }
    else if (MonsterName == FName("Krug"))
    {
        DrawSize = FVector2D(130.0f, 8.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 170.0f);
    }
    else if (MonsterName == FName("Red") || MonsterName == FName("Blue"))
    {
        DrawSize = FVector2D(180.0f, 10.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 220.0f);
    }
    else if (MonsterName == FName("Baron") || MonsterName == FName("baron") ||
        MonsterName == FName("BaronNashor") || MonsterName == FName("Baron_Nashor") ||
        MonsterName == FName("baron_nashor"))
    {
        DrawSize = FVector2D(260.0f, 14.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 420.0f);
    }
    else if (MonsterName == FName("Atakhan") || MonsterName == FName("atakhan") ||
        MonsterName == FName("Atakan") || MonsterName == FName("atakan"))
    {
        DrawSize = FVector2D(240.0f, 14.0f);
        RelativeLocation = FVector(0.0f, 0.0f, 380.0f);
    }

    ActorWidget->SetDrawSize(DrawSize);
    ActorWidget->SetRelativeLocation(RelativeLocation);
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
void ULOL_UIComponent::UpdateLevel(const FChampionStat& CurrentStat)
{
    if (ULOL_ChampionWidget* ChampWidgetObj = Cast<ULOL_ChampionWidget>(ActorWidget->GetUserWidgetObject()))
    {
        ChampWidgetObj->SetLevel(CurrentStat.Level);
    }
}

void ULOL_UIComponent::UpdateHPBarImage(UTexture2D* TargetTexture)
{
    UUserWidget* UserWidgetObj = ActorWidget->GetUserWidgetObject();
    if (!UserWidgetObj) return;
    ULOL_MinionWidget* MinionWidgetObj = Cast<ULOL_MinionWidget>(UserWidgetObj);

    if (MinionWidgetObj)
    {
        MinionWidgetObj->SetHPBarImage(TargetTexture);
    }
}
