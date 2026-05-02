// 챔피언 스킬 관련 컴포넌트
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "BaseChampion.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"

UChampion_SkillComponent::UChampion_SkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    static ConstructorHelpers::FObjectFinder<UDataTable> DataTableAsset(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionSkill.Data_ChampionSkill"));
    if (DataTableAsset.Succeeded())
    {
        SkillDataTable = DataTableAsset.Object;
    }
}

void UChampion_SkillComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UChampion_SkillComponent::InitializeSkills()
{
    ABaseChampion* Owner = Cast<ABaseChampion>(GetOwner());
    if (Owner && SkillDataTable)
    {
        FName Name = Owner->GetChampionName();

        auto GetRow = [&](FString Suffix) -> FSkillData* {
            return SkillDataTable->FindRow<FSkillData>(FName(*(Name.ToString() + Suffix)), TEXT(""));
            };

        if (FSkillData* Data = GetRow(TEXT("_Q"))) Q_Data = *Data;
        if (FSkillData* Data = GetRow(TEXT("_W"))) W_Data = *Data;
        if (FSkillData* Data = GetRow(TEXT("_E"))) E_Data = *Data;
        if (FSkillData* Data = GetRow(TEXT("_R"))) R_Data = *Data;
    }
}

bool UChampion_SkillComponent::TryCastSkill(const FSkillData& SkillData, int32 SkillLevel)
{
    ABaseChampion* Owner = Cast<ABaseChampion>(GetOwner());
    if (!Owner || SkillLevel <= 0) return false;

    int32 Idx = FMath::Clamp(SkillLevel - 1, 0, 4);
    float Cost = SkillData.ManaCost[Idx]; 
    float Mp = Owner->StatComponent->GetStat().CurrentMP;

    // 1. 마나 체크
    if (Mp < Cost) return false;

    // 2. 쿨타임 체크 (로직 필요 시 추가)

    // 3. 자원 소모
    Owner->StatComponent->SetMp(Mp - Cost);

    return true;
}
