#include "JungleMonster/BaseJungleMonster.h"

#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "UObject/ConstructorHelpers.h"

ABaseJungleMonster::ABaseJungleMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(50.f, 90.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UDataTable> ResourceDataAssetTable(
		TEXT("/Game/LOL_Data/Data_JungleMonsters/Data_JungleMonsterResource.Data_JungleMonsterResource")
	);
	if (ResourceDataAssetTable.Succeeded())
	{
		DataTable = ResourceDataAssetTable.Object;
	}

	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));
	AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));
	MoveComponent = CreateDefaultSubobject<ULOL_MoveComponent>(TEXT("MoveComponent"));
	LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));

	LifeCycleComponent->bCanRespawn = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABaseJungleMonster::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();
	}
}

void ABaseJungleMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AttackComponent && AttackComponent->CombatTarget)
	{
		AttackComponent->UpdateAttackLogic();
	}

	if (MoveComponent)
	{
		MoveComponent->UpdateMovement(DeltaTime);
	}
}

float ABaseJungleMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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

void ABaseJungleMonster::SetJungleMonsterData(FName RowName)
{
	if (!DataTable || RowName.IsNone()) return;

	FJungleMonsterResourceData* Data = DataTable->FindRow<FJungleMonsterResourceData>(RowName, TEXT(""));
	if (!Data) return;

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

	JungleMonsterResource.Portrait = Data->Portrait;
	JungleMonsterResource.AttackMontage = Data->AttackMontage;
	JungleMonsterResource.ProjectileMesh = Data->ProjectileMesh;
}
