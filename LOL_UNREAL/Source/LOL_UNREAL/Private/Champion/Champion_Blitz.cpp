#include "Champion/Champion_Blitz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Blitz::AChampion_Blitz()
{
	ChampionName = TEXT("Blitzcrank");
}
void AChampion_Blitz::Skill_Q() {}
void AChampion_Blitz::Skill_W() {}
void AChampion_Blitz::Skill_E() {}
void AChampion_Blitz::Skill_R() {}