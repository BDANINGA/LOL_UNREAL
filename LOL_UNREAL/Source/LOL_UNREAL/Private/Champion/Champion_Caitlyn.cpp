#include "Champion/Champion_Caitlyn.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Caitlyn::AChampion_Caitlyn()
{
    ChampionName = TEXT("Caitlyn");
}

void AChampion_Caitlyn::Skill_Q()
{
}

void AChampion_Caitlyn::Skill_W()
{
}

void AChampion_Caitlyn::Skill_E()
{
}

void AChampion_Caitlyn::Skill_R()
{
}
