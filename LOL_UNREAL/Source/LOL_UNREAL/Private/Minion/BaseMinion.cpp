// 미니언 기본 뼈대
#include "Minion/BaseMinion.h"
#include "Minion/LOL_MinionAIController.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"

#include "Engine/DamageEvents.h"

#include "UObject/ConstructorHelpers.h"

ABaseMinion::ABaseMinion()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComponent->InitCapsuleSize(35.f, 70.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetHiddenInGame(false);
	RootComponent = CapsuleComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -70.f)); 
	MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); 
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MinionMeshRef(TEXT("/Game/Level/blue_minion_melee/blue_minion_skeletal_mesh/unreal_blue_minionmelee_TEST.unreal_blue_minionmelee_TEST"));
	if (MinionMeshRef.Succeeded())
	{
		MeshComponent->SetSkeletalMesh(MinionMeshRef.Object);
	}

	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));
	//AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));
	MoveComponent = CreateDefaultSubobject<ULOL_MoveComponent>(TEXT("MoveComponent"));
	//LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));
	//UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));

	AIControllerClass = ALOL_MinionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABaseMinion::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();
	}
}

void ABaseMinion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (MoveComponent)
	{
		MoveComponent->UpdateMovement(DeltaTime);
	}
}

float ABaseMinion::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (StatComponent)
	{
		EDamageType DamageType = EDamageType::Physical;
		if (DamageEvent.DamageTypeClass == ULOL_DamageMagic::StaticClass())
		{
			DamageType = EDamageType::Magic;
		}
		else if (DamageEvent.DamageTypeClass == ULOL_DamageTrueDamage::StaticClass())
		{
			DamageType = EDamageType::TrueDamage;
		}
		ActualDamage = StatComponent->ApplyDamage(ActualDamage, DamageType);
	}

	return ActualDamage;
}