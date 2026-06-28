#include "JungleMonster/BaseJungleMonster.h"

#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABaseJungleMonster::ABaseJungleMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	ACharacter::SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(50.f, 90.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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
	UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));

	LifeCycleComponent->bCanRespawn = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABaseJungleMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseJungleMonster, JungleMonsterName);
}

void ABaseJungleMonster::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();

	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetStat().MoveSpeed;
		}
	}

	if (StatComponent && UIComponent)
	{
		StatComponent->OnHpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateHpFromStat);
		UIComponent->SetMaxHp(StatComponent->GetStat().MaxHP);
		UIComponent->UpdateHpFromStat(StatComponent->GetCurrentHP());
	}
}

void ABaseJungleMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCrowdControlled)
	{
		return;
	}

	if (bStationaryMonster && bReturningToSpawn)
	{
		bReturningToSpawn = false;
		SetActorLocation(SpawnLocation);
		SetActorRotation(SpawnRotation);
	}

	if (bReturningToSpawn)
	{
		UpdateReturnToSpawn(DeltaTime);
		return;
	}

	if (AttackComponent && AttackComponent->CombatTarget)
	{
		if (ShouldResetLeash())
		{
			StartReturnToSpawn();
			return;
		}

		AttackComponent->UpdateAttackLogic();
	}

	if (MoveComponent)
	{
		if (!bStationaryMonster)
		{
			MoveComponent->UpdateMovement(DeltaTime);
		}
	}
}

void ABaseJungleMonster::ApplyCrowdControl(float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	bCrowdControlled = true;

	if (AttackComponent)
	{
		AttackComponent->ReceivedCrowdControl();
	}
	if (MoveComponent)
	{
		MoveComponent->StopMovement();
	}
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	if (UCharacterMovementComponent* JungleMovement = GetCharacterMovement())
	{
		JungleMovement->StopMovementImmediately();
		JungleMovement->CurrentRootMotion.Clear();
	}

	GetWorldTimerManager().ClearTimer(CrowdControlTimerHandle);
	GetWorldTimerManager().SetTimer(
		CrowdControlTimerHandle,
		this,
		&ABaseJungleMonster::ClearCrowdControl,
		FMath::Max(0.05f, Duration),
		false);
}

void ABaseJungleMonster::ClearCrowdControl()
{
	bCrowdControlled = false;
	if (AttackComponent)
	{
		AttackComponent->ResetAttack();
	}
}

float ABaseJungleMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ULOL_StateComponent* SourceState = EventInstigator && EventInstigator->GetPawn()
		? EventInstigator->GetPawn()->FindComponentByClass<ULOL_StateComponent>()
		: nullptr;
	if (!SourceState && DamageCauser)
	{
		SourceState = DamageCauser->FindComponentByClass<ULOL_StateComponent>();
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

	if (ActualDamage > 0.0f && AttackComponent)
	{
		AActor* Attacker = EventInstigator ? EventInstigator->GetPawn() : nullptr;
		if (!Attacker)
		{
			Attacker = DamageCauser;
		}

		AttackComponent->SetCombatTarget(Attacker);
	}

	return ActualDamage;
}

void ABaseJungleMonster::SetJungleMonsterData(FName RowName)
{
	if (!DataTable || RowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Jungle monster data setup failed. DataTable=%s RowName=%s"),
			DataTable ? TEXT("Valid") : TEXT("Null"),
			*RowName.ToString());
		return;
	}

	JungleMonsterName = RowName;

	FName ResourceRowName = RowName;
	FJungleMonsterResourceData* Data = DataTable->FindRow<FJungleMonsterResourceData>(ResourceRowName, TEXT(""));
	if ((!Data || !Data->Mesh) && RowName == FName("Atakhan"))
	{
		const TArray<FName> AtakhanAliases = {
			FName("atakhan"),
			FName("Atakan"),
			FName("atakan")
		};

		for (const FName& AtakhanAlias : AtakhanAliases)
		{
			if (FJungleMonsterResourceData* AtakhanData =
				DataTable->FindRow<FJungleMonsterResourceData>(AtakhanAlias, TEXT("")))
			{
				Data = AtakhanData;
				ResourceRowName = AtakhanAlias;
				break;
			}
		}
	}
	else if ((!Data || !Data->Mesh) && RowName == FName("atakhan"))
	{
		if (FJungleMonsterResourceData* AtakhanData =
			DataTable->FindRow<FJungleMonsterResourceData>(FName("Atakhan"), TEXT("")))
		{
			Data = AtakhanData;
			ResourceRowName = FName("Atakhan");
		}
	}
	else if ((!Data || !Data->Mesh) && (RowName == FName("Atakan") || RowName == FName("atakan")))
	{
		const TArray<FName> AtakhanAliases = {
			FName("Atakhan"),
			FName("atakhan")
		};

		for (const FName& AtakhanAlias : AtakhanAliases)
		{
			if (FJungleMonsterResourceData* AtakhanData =
				DataTable->FindRow<FJungleMonsterResourceData>(AtakhanAlias, TEXT("")))
			{
				Data = AtakhanData;
				ResourceRowName = AtakhanAlias;
				break;
			}
		}
	}
	else if ((!Data || !Data->Mesh) && RowName == FName("Baron"))
	{
		const TArray<FName> BaronAliases = {
			FName("baron"),
			FName("BaronNashor"),
			FName("Baron_Nashor"),
			FName("baron_nashor")
		};

		for (const FName& BaronAlias : BaronAliases)
		{
			if (FJungleMonsterResourceData* BaronData =
				DataTable->FindRow<FJungleMonsterResourceData>(BaronAlias, TEXT("")))
			{
				Data = BaronData;
				ResourceRowName = BaronAlias;
				break;
			}
		}
	}
	else if ((!Data || !Data->Mesh) && RowName == FName("baron"))
	{
		if (FJungleMonsterResourceData* BaronData =
			DataTable->FindRow<FJungleMonsterResourceData>(FName("Baron"), TEXT("")))
		{
			Data = BaronData;
			ResourceRowName = FName("Baron");
		}
	}
	if ((!Data || !Data->Mesh) && RowName == FName("Raptor"))
	{
		if (FJungleMonsterResourceData* RazorbeakData =
			DataTable->FindRow<FJungleMonsterResourceData>(FName("Razorbeak"), TEXT("")))
		{
			Data = RazorbeakData;
			ResourceRowName = FName("Razorbeak");
		}
	}
	else if ((!Data || !Data->Mesh) && RowName == FName("Razorbeak"))
	{
		if (FJungleMonsterResourceData* RaptorData =
			DataTable->FindRow<FJungleMonsterResourceData>(FName("Raptor"), TEXT("")))
		{
			Data = RaptorData;
			ResourceRowName = FName("Raptor");
		}
	}

	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("Jungle monster resource row not found. RowName=%s"), *RowName.ToString());
		return;
	}

	if (Data->Mesh)
	{
		GetMesh()->SetSkeletalMesh(Data->Mesh);
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		GetMesh()->SetRelativeScale3D(GetMeshScaleForMonster(ResourceRowName));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Jungle monster mesh missing. RowName=%s"), *RowName.ToString());
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

void ABaseJungleMonster::InitializeJungleMonster(FName RowName)
{
	if (RowName.IsNone()) return;

	JungleMonsterName = RowName;
	bStationaryMonster =
		RowName == FName("Atakhan") ||
		RowName == FName("atakhan") ||
		RowName == FName("Atakan") ||
		RowName == FName("atakan") ||
		RowName == FName("Baron") ||
		RowName == FName("baron") ||
		RowName == FName("BaronNashor") ||
		RowName == FName("Baron_Nashor") ||
		RowName == FName("baron_nashor");
	SetJungleMonsterData(RowName);

	if (HasAuthority() && StatComponent)
	{
		StatComponent->InitializeStat();
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = StatComponent->GetStat().MoveSpeed;
		}
	}

	if (StatComponent && UIComponent)
	{
		UIComponent->SetMaxHp(StatComponent->GetStat().MaxHP);
		UIComponent->UpdateHpFromStat(StatComponent->GetCurrentHP());
	}

	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();
}

void ABaseJungleMonster::OnRep_JungleMonsterName()
{
	if (JungleMonsterName.IsNone())
	{
		return;
	}

	bStationaryMonster =
		JungleMonsterName == FName("Atakhan") ||
		JungleMonsterName == FName("atakhan") ||
		JungleMonsterName == FName("Atakan") ||
		JungleMonsterName == FName("atakan") ||
		JungleMonsterName == FName("Baron") ||
		JungleMonsterName == FName("baron") ||
		JungleMonsterName == FName("BaronNashor") ||
		JungleMonsterName == FName("Baron_Nashor") ||
		JungleMonsterName == FName("baron_nashor");
	SetJungleMonsterData(JungleMonsterName);

	if (StatComponent)
	{
		StatComponent->InitializeStat();
	}

	if (StatComponent && UIComponent)
	{
		UIComponent->SetMaxHp(StatComponent->GetStat().MaxHP);
		UIComponent->UpdateHpFromStat(StatComponent->GetCurrentHP());
	}
}

FVector ABaseJungleMonster::GetMeshScaleForMonster(FName RowName) const
{
	if (RowName == FName("Red"))
	{
		return FVector(1.6f);
	}

	return FVector(1.0f);
}

void ABaseJungleMonster::StartReturnToSpawn()
{
	bReturningToSpawn = true;

	if (AttackComponent)
	{
		AttackComponent->SetCombatTarget(nullptr);
		AttackComponent->HitTarget = nullptr;
		AttackComponent->ResetAttack();
		GetWorldTimerManager().ClearTimer(AttackComponent->AttackTimerHandle);
		GetWorldTimerManager().ClearTimer(AttackComponent->AttackHitTimerHandle);
	}

	if (StateComponent)
	{
		StateComponent->RemoveStatusTag(LOLTags::State_Attacking);
		if (!bStationaryMonster)
		{
			StateComponent->AddStatusTag(LOLTags::State_Moving);
		}
	}

	if (MoveComponent)
	{
		MoveComponent->TargetLocation = SpawnLocation;
	}

	if (bStationaryMonster)
	{
		bReturningToSpawn = false;
		SetActorLocation(SpawnLocation);
		SetActorRotation(SpawnRotation);
	}
}

void ABaseJungleMonster::UpdateReturnToSpawn(float DeltaTime)
{
	FVector Direction = SpawnLocation - GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.Size2D() <= ReturnAcceptanceRadius)
	{
		bReturningToSpawn = false;
		SetActorLocation(SpawnLocation);
		SetActorRotation(SpawnRotation);

		if (MoveComponent)
		{
			MoveComponent->StopMovement();
		}
		if (StateComponent)
		{
			StateComponent->RemoveStatusTag(LOLTags::State_Moving);
		}
		return;
	}

	const FVector MoveDirection = Direction.GetSafeNormal2D();
	AddMovementInput(MoveDirection, 1.0f);
	SetActorRotation(MoveDirection.Rotation());
}

bool ABaseJungleMonster::ShouldResetLeash() const
{
	if (!AttackComponent || !AttackComponent->CombatTarget)
	{
		return false;
	}

	const float MonsterDistanceFromSpawn = FVector::Dist2D(GetActorLocation(), SpawnLocation);
	const float TargetDistanceFromSpawn = FVector::Dist2D(AttackComponent->CombatTarget->GetActorLocation(), SpawnLocation);

	return MonsterDistanceFromSpawn > LeashRadius || TargetDistanceFromSpawn > LeashRadius;
}
