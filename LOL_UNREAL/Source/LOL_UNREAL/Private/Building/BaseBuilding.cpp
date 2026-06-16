#include "Building/BaseBuilding.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseBuilding::ABaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleHalfHeight(150.f);
	CapsuleComponent->SetCapsuleRadius(100.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn")); 

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));
	AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));
	LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));
	UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent)
	{
		StatComponent->InitializeStat();
	}
}