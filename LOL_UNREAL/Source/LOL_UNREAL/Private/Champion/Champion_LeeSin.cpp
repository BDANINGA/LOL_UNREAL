#include "Champion/Champion_LeeSin.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_LeeSin::AChampion_LeeSin()
{
    ChampionName = TEXT("Leesin");
}

void AChampion_LeeSin::Skill_Q()
{
}

void AChampion_LeeSin::Skill_W()
{
}

void AChampion_LeeSin::Skill_E()
{
}

void AChampion_LeeSin::Skill_R()
{
}
