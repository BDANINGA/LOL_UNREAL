// 챔피언 스킬 관련 컴포넌트
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "BaseChampion.h"
#include "LOL_HUD.h"

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
}

bool UChampion_SkillComponent::TryCastSkill(FName SkillName, int32 SkillLevel)
{
    if (!Owner || SkillLevel <= 0) return false;

    FSkillData* SkillData = nullptr;

    if (SkillName == "Q") SkillData = &Q_Data;
    if (SkillName == "W") SkillData = &W_Data;
    if (SkillName == "E") SkillData = &E_Data;
    if (SkillName == "R") SkillData = &R_Data;

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime < SkillData->CooldownEndTime) return false;

    int32 SkillLevelIdx = FMath::Clamp(SkillLevel - 1, 0, 4);

    float Cost = SkillData->ManaCost[SkillLevelIdx];
    float Mp = Owner->StatComponent->GetCurrentMP();

    if (Mp < Cost) return false;

    Owner->StatComponent->SetMP(Mp - Cost);

    float BaseCooldown = SkillData->Cooldown[SkillLevelIdx];
    float Haste = Owner->StatComponent->GetStat().AbilityHaste;
    float FinalCooldown = BaseCooldown * (100.f / (100.f + Haste));

    SkillData->CooldownEndTime = CurrentTime + FinalCooldown;

    Client_UpdateHUDCooldown(SkillName, SkillData->CooldownEndTime, FinalCooldown);

    return true;
}