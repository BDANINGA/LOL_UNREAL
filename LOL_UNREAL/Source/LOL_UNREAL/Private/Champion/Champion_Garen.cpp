#include "Champion/Champion_Garen.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Garen::AChampion_Garen()
{
    ChampionName = TEXT("Garen");
    SetChampionData(ChampionName);
}

void AChampion_Garen::SetChampionData(FName RowName)
{
    static ConstructorHelpers::FObjectFinder<UDataTable> ChampionResource(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionResource.Data_ChampionResource"));
    if (ChampionResource.Succeeded())
    {
        UDataTable* DataTable = ChampionResource.Object;

        FChampionData* Data = DataTable->FindRow<FChampionData>(RowName, TEXT(""));

        if (Data)
        {
            if (Data->Mesh)
            {
                GetMesh()->SetSkeletalMesh(Data->Mesh);
                GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
                GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
            }
            if (Data->AnimBlueprint)
            {
                GetMesh()->SetAnimInstanceClass(Data->AnimBlueprint);
                GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            }
            Portrait = Data->Portrait;
            Portrait_Circle = Data->Portrait_Circle;
            Portrait_Loading = Data->Portrait_Loading;

            SkillQ_Image = Data->SkillQ_Image;
            SkillW_Image = Data->SkillW_Image;
            SkillE_Image = Data->SkillE_Image;
            SkillR_Image = Data->SkillR_Image;
            SkillP_Image = Data->SkillP_Image;

            AttackMontage = Data->AttackMontage;
            DeathMontage = Data->DeathMontage;
            QMontage = Data->QMontage;
            WMontage = Data->WMontage;
            EMontage = Data->EMontage;
            RMontage = Data->RMontage;
            PMontage = Data->PMontage;
        }
    }
}

void AChampion_Garen::Skill_Q()
{
}

void AChampion_Garen::Skill_W()
{
}

void AChampion_Garen::Skill_E()
{
}

void AChampion_Garen::Skill_R()
{
}
