// 미니언 기본 뼈대
#include "Minion/BaseMinion.h"
#include "Minion/LOL_MinionAIController.h"

#include "BaseChampion.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StateComponent.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

#include "UObject/ConstructorHelpers.h"

ABaseMinion::ABaseMinion()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));

	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UDataTable> ResourceDataAssetTable(TEXT("/Game/LOL_Data/Data_Minions/Data_MinionResource.Data_MinionResource"));
	if (ResourceDataAssetTable.Succeeded()) DataTable = ResourceDataAssetTable.Object;

	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));
	AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));
	MoveComponent = CreateDefaultSubobject<ULOL_MoveComponent>(TEXT("MoveComponent"));
	LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));
	UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));

	AIControllerClass = ALOL_MinionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABaseMinion::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();

		if (GetCharacterMovement() && StatComponent)
		{
			GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetStat().MoveSpeed;
		}
	}
	if (StatComponent && UIComponent)
	{
		StatComponent->OnHpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateHpFromStat);
		StateComponent->OnStateTagsChanged.AddUObject(this, &ABaseMinion::UpdateTeamVisual);
		//StatComponent->OnHpZero.AddDynamic(LifeCycleComponent, &ULOL_LifeCycleComponent::Server_HandleDeath);

		UIComponent->SetMaxHp(StatComponent->GetStat().MaxHP);
		UIComponent->UpdateHpFromStat(StatComponent->GetCurrentHP());
	}
	UpdateTeamVisual();
}

void ABaseMinion::Tick(float DeltaTime)
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

float ABaseMinion::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ULOL_StateComponent* SourceState = DamageCauser ? DamageCauser->FindComponentByClass<ULOL_StateComponent>() : nullptr;
	if (!SourceState && EventInstigator && EventInstigator->GetPawn())
	{
		SourceState = EventInstigator->GetPawn()->FindComponentByClass<ULOL_StateComponent>();
	}

	if (StateComponent && SourceState && !SourceState->IsEnemy(StateComponent))
	{
		return 0.0f;
	}

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
		ActualDamage = StatComponent->ApplyDamage(ActualDamage, DamageType, EventInstigator, DamageCauser);
	}

	return ActualDamage;
}

void ABaseMinion::SetMinionData(FName RowName)
{
	FMinionResourceData* Data = DataTable->FindRow<FMinionResourceData>(RowName, TEXT(""));

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
		MinionResource.Portrait = Data->Portrait;

		MinionResource.AttackMontage = Data->AttackMontage;

		MinionResource.AllyProjectileNiagara = Data->AllyProjectileNiagara;
		MinionResource.EnemyProjectileNiagara = Data->EnemyProjectileNiagara;

		MinionResource.AllyTexture = Data->AllyTexture;
		MinionResource.EnemyTexture = Data->EnemyTexture;

		MinionResource.AllyHPBarImage = Data->AllyHPBarImage;
		MinionResource.EnemyHPBarImage = Data->EnemyHPBarImage;
	}
}

void ABaseMinion::MoveToNextWaypoint()
{
	CurrentPathIndex++;
	if (PathPoints.IsValidIndex(CurrentPathIndex))
	{
		MoveComponent->SetMoveTarget(PathPoints[CurrentPathIndex], nullptr);
	}
}
void ABaseMinion::UpdateTeamVisual()
{
	if (!StateComponent || !MinionResource.AllyTexture || !MinionResource.EnemyTexture) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	ABaseChampion* LocalPlayer = Cast<ABaseChampion>(PC->GetPawn());

	if (!LocalPlayer || !LocalPlayer->StateComponent)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ABaseMinion::UpdateTeamVisual);
		return;
	}

	bool bIsEnemy = LocalPlayer->StateComponent->IsEnemy(StateComponent);

	UTexture2D* TargetTexture = bIsEnemy ? MinionResource.EnemyTexture : MinionResource.AllyTexture;

	UMaterialInstanceDynamic* DynamicMaterial = GetMesh()->CreateDynamicMaterialInstance(0);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(FName("TeamTexture"), TargetTexture);
	}

	if (UIComponent)
	{
		UTexture2D* TargetUITexture = bIsEnemy ? MinionResource.EnemyHPBarImage : MinionResource.AllyHPBarImage;

		UIComponent->UpdateHPBarImage(TargetUITexture);
	}
}

void ABaseMinion::Multicast_SetTargetAndPlayMontage_Implementation(UAnimMontage* AnimMontage, float InplayRate, FRotator TargetRotation)
{
	SetActorRotation(TargetRotation);

	if (AnimMontage) PlayAnimMontage(AnimMontage, InplayRate);
}