// 챔피언 스킬 관련 컴포넌트
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "BaseChampion.h"
#include "LOL_HUD.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

UChampion_SkillComponent::UChampion_SkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    static ConstructorHelpers::FObjectFinder<UDataTable> DataTableAsset(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionSkill.Data_ChampionSkill"));
    if (DataTableAsset.Succeeded())
    {
        SkillDataTable = DataTableAsset.Object;
    }
}

void UChampion_SkillComponent::BeginPlay()
{
	Super::BeginPlay();

    Owner = Cast<ABaseChampion>(GetOwner());
}

void UChampion_SkillComponent::Client_UpdateHUDCooldown_Implementation(FName SkillName, float CoolLocalEndTime, float CoolEndTime)
{
    if (Owner && Owner->IsLocallyControlled())
    {
        APlayerController* PC = Cast<APlayerController>(Owner->GetController());
        if (PC)
        {
            ALOL_HUD* MyHUD = Cast<ALOL_HUD>(PC->GetHUD());
            if (MyHUD)
            {
                MyHUD->UpdateSkillCoolDown(SkillName, CoolLocalEndTime, CoolEndTime);
            }
        }
    }
}

void UChampion_SkillComponent::InitializeSkills()
{
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

    if (Owner && Owner->HasAuthority() && AvailableSkillPoints <= 0 &&
        QSkillLevel + WSkillLevel + ESkillLevel + RSkillLevel <= 0)
    {
        const int32 ChampionLevel = Owner->StatComponent
            ? Owner->StatComponent->GetStat().Level
            : 1;
        AvailableSkillPoints = FMath::Clamp(ChampionLevel, 1, 18);
    }
}

bool UChampion_SkillComponent::TryCastSkill(FName SkillName, int32 SkillLevel)
{
    if (!Owner) return false;

    FSkillData* SkillData = nullptr;

    if (SkillName == "Q") SkillData = &Q_Data;
    if (SkillName == "W") SkillData = &W_Data;
    if (SkillName == "E") SkillData = &E_Data;
    if (SkillName == "R") SkillData = &R_Data;
    if (!SkillData) return false;

    const int32 LearnedSkillLevel = GetSkillLevel(SkillName);
    if (LearnedSkillLevel <= 0) return false;

    SkillLevel = LearnedSkillLevel;

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime < SkillData->CooldownEndTime) return false;

    const int32 HighestDataIndex = FMath::Max(
        0,
        FMath::Min(
            4,
            FMath::Max(SkillData->ManaCost.Num(), SkillData->Cooldown.Num()) - 1
        )
    );
    int32 SkillLevelIdx = FMath::Clamp(SkillLevel - 1, 0, HighestDataIndex);

    float Cost = SkillData->ManaCost.IsValidIndex(SkillLevelIdx)
        ? SkillData->ManaCost[SkillLevelIdx]
        : 0.0f;
    float Mp = Owner->StatComponent->GetCurrentMP();

    if (Mp < Cost) return false;

    Owner->StatComponent->SetMP(Mp - Cost);

    float BaseCooldown = SkillData->Cooldown.IsValidIndex(SkillLevelIdx)
        ? SkillData->Cooldown[SkillLevelIdx]
        : 0.0f;
    float Haste = Owner->StatComponent->GetStat().AbilityHaste;
    float FinalCooldown = BaseCooldown * (100.f / (100.f + Haste));

    SkillData->CooldownEndTime = CurrentTime + FinalCooldown;

    Client_UpdateHUDCooldown(SkillName, SkillData->CooldownEndTime, FinalCooldown);

    return true;
}

int32 UChampion_SkillComponent::GetSkillLevel(FName SkillName) const
{
    if (SkillName == "Q") return QSkillLevel;
    if (SkillName == "W") return WSkillLevel;
    if (SkillName == "E") return ESkillLevel;
    if (SkillName == "R") return RSkillLevel;
    return 0;
}

int32 UChampion_SkillComponent::GetSkillLevelIndex(FName SkillName) const
{
    return FMath::Max(GetSkillLevel(SkillName) - 1, 0);
}

int32& UChampion_SkillComponent::GetMutableSkillLevel(FName SkillName)
{
    if (SkillName == "Q") return QSkillLevel;
    if (SkillName == "W") return WSkillLevel;
    if (SkillName == "E") return ESkillLevel;
    return RSkillLevel;
}

int32 UChampion_SkillComponent::GetMaxAllowedSkillLevel(
    FName SkillName,
    int32 ChampionLevel) const
{
    if (SkillName == "R")
    {
        if (ChampionLevel >= 16) return 3;
        if (ChampionLevel >= 11) return 2;
        if (ChampionLevel >= 6) return 1;
        return 0;
    }

    if (SkillName == "Q" || SkillName == "W" || SkillName == "E")
    {
        if (ChampionLevel >= 9) return 5;
        if (ChampionLevel >= 7) return 4;
        if (ChampionLevel >= 5) return 3;
        if (ChampionLevel >= 3) return 2;
        if (ChampionLevel >= 1) return 1;
    }

    return 0;
}

bool UChampion_SkillComponent::CanLevelUpSkill(FName SkillName) const
{
    if (!Owner || !Owner->StatComponent || AvailableSkillPoints <= 0)
    {
        return false;
    }

    const int32 ChampionLevel = Owner->StatComponent->GetStat().Level;
    const int32 CurrentSkillLevel = GetSkillLevel(SkillName);
    const int32 MaxAllowedSkillLevel =
        GetMaxAllowedSkillLevel(SkillName, ChampionLevel);

    return CurrentSkillLevel < MaxAllowedSkillLevel;
}

void UChampion_SkillComponent::AddSkillPointForChampionLevel(int32 ChampionLevel)
{
    if (!Owner || !Owner->HasAuthority() || ChampionLevel <= 1)
    {
        return;
    }

    ++AvailableSkillPoints;
}

bool UChampion_SkillComponent::Server_LevelUpSkill_Validate(FName SkillName)
{
    return true;
}

void UChampion_SkillComponent::Server_LevelUpSkill_Implementation(FName SkillName)
{
    if (!CanLevelUpSkill(SkillName))
    {
        return;
    }

    int32& SkillLevel = GetMutableSkillLevel(SkillName);
    ++SkillLevel;
    --AvailableSkillPoints;
    ApplyCurrentSkillRankData(SkillName);
}

void UChampion_SkillComponent::ApplyCurrentSkillRankData(FName SkillName)
{
    if (SkillName == "Q")
    {
        CopyCurrentRankToFirstIndex(Q_Data, QSkillLevel);
        return;
    }
    if (SkillName == "W")
    {
        CopyCurrentRankToFirstIndex(W_Data, WSkillLevel);
        return;
    }
    if (SkillName == "E")
    {
        CopyCurrentRankToFirstIndex(E_Data, ESkillLevel);
        return;
    }
    if (SkillName == "R")
    {
        CopyCurrentRankToFirstIndex(R_Data, RSkillLevel);
    }
}

void UChampion_SkillComponent::CopyCurrentRankToFirstIndex(
    FSkillData& SkillData,
    int32 SkillLevel)
{
    const int32 SkillLevelIndex = FMath::Max(SkillLevel - 1, 0);

    auto CopyArrayValue = [SkillLevelIndex](TArray<float>& Values)
    {
        if (!Values.IsValidIndex(0))
        {
            return;
        }

        const int32 SourceIndex = FMath::Clamp(
            SkillLevelIndex,
            0,
            Values.Num() - 1);
        Values[0] = Values[SourceIndex];
    };

    CopyArrayValue(SkillData.BaseDamage);
    CopyArrayValue(SkillData.ManaCost);
    CopyArrayValue(SkillData.Cooldown);
    CopyArrayValue(SkillData.Range);
    CopyArrayValue(SkillData.Duration);
    CopyArrayValue(SkillData.SecondaryValue);
}

void UChampion_SkillComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UChampion_SkillComponent, QSkillLevel);
    DOREPLIFETIME(UChampion_SkillComponent, WSkillLevel);
    DOREPLIFETIME(UChampion_SkillComponent, ESkillLevel);
    DOREPLIFETIME(UChampion_SkillComponent, RSkillLevel);
    DOREPLIFETIME(UChampion_SkillComponent, AvailableSkillPoints);
}
