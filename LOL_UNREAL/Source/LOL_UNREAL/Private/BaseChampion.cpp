// 챔피언의 기본 설정
// ----------------------------------------------------------------------------------
#include "BaseChampion.h"
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"
#include "VisionManager/VisionManager.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_VisionComponent.h"

#include "GamePlayTag/LOL_GamePlayTags.h"
#include "Component/Champion_SkillComponent.h"

#include "Building/BaseBuilding.h"
#include "UObject/ConstructorHelpers.h"

ABaseChampion::ABaseChampion()
{
	PrimaryActorTick.bCanEverTick = true;

	// Stat
	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));

	// Attack
	AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));

	// Move
	MoveComponent = CreateDefaultSubobject<ULOL_MoveComponent>(TEXT("MoveComponent"));

	// LifeCycle
	LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));

	// UI
	UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));

	// State
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));

	// Vision
	VisionComponent = CreateDefaultSubobject<ULOL_VisionComponent>(TEXT("VisionComponent"));

	// Skill
	SkillComponent = CreateDefaultSubobject<UChampion_SkillComponent>(TEXT("SkillComponent"));

	static ConstructorHelpers::FObjectFinder<UDataTable> ResourceDataAssetTable(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionResource.Data_ChampionResource"));
	if (ResourceDataAssetTable.Succeeded()) DataTable = ResourceDataAssetTable.Object;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RecallEffectAsset(TEXT("/Game/UI/recall/StaticMeshes/ns_recall.ns_recall"));
	if (RecallEffectAsset.Succeeded()) RecallEffectSystem = RecallEffectAsset.Object;

	// 캡슐 컴포넌트의 콜리전 설정
	GetCapsuleComponent()->InitCapsuleSize(60.f, 130.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
	AttackRangeSphere->SetupAttachment(RootComponent);
	AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 캐릭터가 컨트롤러의 회전값을 직접 상속받지 않도록 확실히 차단
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 이동 설정
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		// 물리 오브젝트 상호작용 끄기
		Movement->bEnablePhysicsInteraction = false;

		// 불필요한 이동 모드 차단 (수영, 비행, 웅크리기)
		Movement->DefaultWaterMovementMode = EMovementMode::MOVE_None;
		Movement->GetNavAgentPropertiesRef().bCanSwim = false;
		Movement->GetNavAgentPropertiesRef().bCanFly = false;
		Movement->GetNavAgentPropertiesRef().bCanCrouch = false;

		// 수동 점프 차단
		Movement->GetNavAgentPropertiesRef().bCanJump = false;

		// 낭떠러지 추락 방지
		Movement->bCanWalkOffLedges = true;
		Movement->LedgeCheckThreshold = 0.f;

		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.f, 640.f, 0.f);
		Movement->bConstrainToPlane = true;
		Movement->bSnapToPlaneAtStart = true;
	}

	// 위치와 회전을 모두 복제하도록 설정
	bReplicates = true;
	ACharacter::SetReplicateMovement(true);
}
void ABaseChampion::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
	}

	RecallHomeLocation = GetActorLocation();
	RecallHomeRotation = GetActorRotation();

	if (StatComponent) StatComponent->InitializeStat();
	if (SkillComponent) SkillComponent->InitializeSkills();

	// UI 설정
	if (StatComponent && UIComponent)
	{
		AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseChampion::OnEnemyEnterRange);
		AttackRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseChampion::OnEnemyLeaveRange);
		AttackRangeSphere->SetSphereRadius(400.f);

		StatComponent->OnHpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateHpFromStat);
		StatComponent->OnMpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateMpFromStat);
		StatComponent->OnStatChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateLevel);
		//StatComponent->OnHpZero.AddDynamic(LifeCycleComponent, &ULOL_LifeCycleComponent::Server_HandleDeath);
		
		UIComponent->SetMaxHp(StatComponent->GetStat().MaxHP);
		UIComponent->SetMaxMp(StatComponent->GetStat().MaxMP);

		UIComponent->UpdateHpFromStat(StatComponent->GetCurrentHP());
		UIComponent->UpdateMpFromStat(StatComponent->GetCurrentMP());
		if (IsLocallyControlled())
		{
			APlayerController* PC = Cast<APlayerController>(GetController());
			if (PC)
			{
				ALOL_HUD* MyHUD = Cast<ALOL_HUD>(PC->GetHUD());
				if (MyHUD && MyHUD->MainHUDWidget)
				{
					StatComponent->OnStatChanged.AddUObject(MyHUD, &ALOL_HUD::UpdateStat);
					StatComponent->OnHpChanged.AddUObject(MyHUD, &ALOL_HUD::UpdateHP);
					StatComponent->OnMpChanged.AddUObject(MyHUD, &ALOL_HUD::UpdateMP);
					StatComponent->OnGoldChanged.AddUObject(MyHUD, &ALOL_HUD::UpdateGold);
					StatComponent->OnEXPChanged.AddUObject(MyHUD, &ALOL_HUD::UpdateEXP);
					MyHUD->UpdateAll_Images(this);
					MyHUD->UpdateStat(StatComponent->GetStat());
					MyHUD->UpdateHP(StatComponent->GetCurrentHP());
					MyHUD->UpdateMP(StatComponent->GetCurrentMP());
				}
			}
		}
	}
	AVisionManager* Manager =
		Cast<AVisionManager>(
			UGameplayStatics::GetActorOfClass(
				GetWorld(),
				AVisionManager::StaticClass()));

	if (Manager)
	{
		Manager->RegisterActor(this);
	}
}

void ABaseChampion::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		if (StateComponent && StateComponent->HasStatusTag(LOLTags::Team_Blue))
		{
			TeamId = 0;
		}
		else if (StateComponent && StateComponent->HasStatusTag(LOLTags::Team_Red))
		{
			TeamId = 1;
		}
		else
		{
			// Listen-server host gets team 0, remote clients get team 1 for now.
			TeamId = NewController && NewController->IsLocalController() ? 0 : 1;
		}
	}
}

void ABaseChampion::SetVisibleByVision(bool bVisible)
{
	bVisibleByVision = bVisible;
	SetActorHiddenInGame(!bVisible);
}

void ABaseChampion::AddKillCount()
{
	if (HasAuthority())
	{
		++KillCount;
	}
}

void ABaseChampion::AddDeathCount()
{
	if (HasAuthority())
	{
		++DeathCount;
	}
}

void ABaseChampion::AddAssistCount()
{
	if (HasAuthority())
	{
		++AssistCount;
	}
}

void ABaseChampion::AddMinionKillCount(int32 Amount)
{
	if (HasAuthority() && Amount > 0)
	{
		MinionKillCount += Amount;
	}
}

void ABaseChampion::SetChampionData(FName RowName)
{
	FChampionResourceData* Data = DataTable->FindRow<FChampionResourceData>(RowName, TEXT(""));

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
		ChampionResource.Portrait = Data->Portrait;
		ChampionResource.Portrait_Circle = Data->Portrait_Circle;
		ChampionResource.Portrait_Loading = Data->Portrait_Loading;

		ChampionResource.SkillQ_Image = Data->SkillQ_Image;
		ChampionResource.SkillW_Image = Data->SkillW_Image;
		ChampionResource.SkillE_Image = Data->SkillE_Image;
		ChampionResource.SkillR_Image = Data->SkillR_Image;
		ChampionResource.SkillP_Image = Data->SkillP_Image;

		ChampionResource.AttackMontage = Data->AttackMontage;
		ChampionResource.QMontage = Data->QMontage;
		ChampionResource.WMontage = Data->WMontage;
		ChampionResource.EMontage = Data->EMontage;
		ChampionResource.RMontage = Data->RMontage;
		ChampionResource.PMontage = Data->PMontage;

		ChampionResource.ProjectileMesh = Data->ProjectileMesh;
	}
}

void ABaseChampion::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseChampion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsKnockedBack)
	{
		return;
	}

	if (StateComponent->HasStatusTag(LOLTags::State_Dead)) return;

	if (AttackComponent && AttackComponent->CombatTarget)
	{
		AttackComponent->UpdateAttackLogic();
	}
	else {
		MoveComponent->UpdateMovement(DeltaTime);
	}
}
float ABaseChampion::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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
		EDamageType Type = EDamageType::Physical; // 기본값

		if (DamageEvent.DamageTypeClass == ULOL_DamageMagic::StaticClass())
		{
			Type = EDamageType::Magic;
		}
		else if (DamageEvent.DamageTypeClass == ULOL_DamageTrueDamage::StaticClass())
		{
			Type = EDamageType::TrueDamage;
		}

		// 2. 판별된 타입을 포함하여 StatComponent 호출
		ActualDamage = StatComponent->ApplyDamage(ActualDamage, Type, EventInstigator, DamageCauser);
	}

	if (HasAuthority() && ActualDamage > 0.0f && bIsRecalling)
	{
		CancelRecall();
	}

	return ActualDamage;
}
void ABaseChampion::OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this) return;

	ULOL_StateComponent* TargetState = OtherActor->FindComponentByClass<ULOL_StateComponent>();

	if (TargetState && StateComponent->IsEnemy(TargetState) && !TargetState->HasStatusTag(LOLTags::State_Dead))
	{
		EnemiesInRange.AddUnique(OtherActor);
	}
}
void ABaseChampion::OnEnemyLeaveRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || !OtherActor) return;

	if (EnemiesInRange.Contains(OtherActor))
	{
		EnemiesInRange.Remove(OtherActor);
	}
}
void ABaseChampion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseChampion, TeamId);
	DOREPLIFETIME(ABaseChampion, KillCount);
	DOREPLIFETIME(ABaseChampion, DeathCount);
	DOREPLIFETIME(ABaseChampion, AssistCount);
	DOREPLIFETIME(ABaseChampion, MinionKillCount);
	DOREPLIFETIME(ABaseChampion, bIsSilenced);
	DOREPLIFETIME(ABaseChampion, bIsRecalling);
}

void ABaseChampion::Server_ExecuteAttackHit_Implementation()
{
	if (AttackComponent)
	{
		AttackComponent->ExecuteAttackHit();
	}
}
bool ABaseChampion::IsEnemyActor(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == this || !StateComponent)
	{
		return false;
	}

	ULOL_StateComponent* TargetState = TargetActor->FindComponentByClass<ULOL_StateComponent>();
	if (TargetState)
	{
		return StateComponent->IsEnemy(TargetState);
	}

	if (const ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor))
	{
		return TargetChampion->TeamId != TeamId;
	}

	return true;
}
void ABaseChampion::ProcessMoveInput(FVector ClickLocation, AActor* TargetActor)
{
	if (bIsRecalling)
	{
		Server_CancelRecall();
		return;
	}

	if (IsMoveInputBlocked()) return;

	FVector ClickDirection = ClickLocation - GetActorLocation();
	ClickDirection.Z = 0.0f;
	if (!ClickDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ClickDirection.Rotation().Yaw, 0.0f));
	}

	const bool bIsAttackTarget =
		TargetActor &&
		TargetActor != this &&
		IsEnemyActor(TargetActor) &&
		TargetActor->FindComponentByClass<ULOL_StateComponent>();

	if (!HasAuthority() && IsLocallyControlled() && MoveComponent && !bIsAttackTarget)
	{
		MoveComponent->bIsSearchAttack = bIsPressA;
		MoveComponent->SetMoveTarget(ClickLocation, TargetActor);
	}

	Server_ProcessMoveInput(ClickLocation, TargetActor, bIsPressA);
}
void ABaseChampion::Server_ProcessMoveInput_Implementation(FVector ClickLocation, AActor* TargetActor, bool bIsSearch)
{
	if (bIsRecalling)
	{
		CancelRecall();
		return;
	}

	if (IsMoveInputBlocked()) return;
	if (bIsKnockedBack) return;
	if (StateComponent->HasStatusTag(LOLTags::State_Dead)) return;
	if (bIsStunned) return;

	FVector ClickDirection = ClickLocation - GetActorLocation();
	ClickDirection.Z = 0.0f;
	if (!ClickDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ClickDirection.Rotation().Yaw, 0.0f));
	}

	MoveComponent->bIsSearchAttack = bIsSearch;

	AActor* AttackTarget = nullptr;
	if (TargetActor && TargetActor != this && IsEnemyActor(TargetActor) && TargetActor->FindComponentByClass<ULOL_StateComponent>())
	{
		AttackTarget = TargetActor;
	}
	else if (TargetActor)
	{
		TargetActor = nullptr;
	}

	if (AttackComponent && !AttackComponent->CanAttack())
	{
		if (AttackTarget == nullptr || AttackTarget != AttackComponent->CombatTarget)
		{
			AttackComponent->CancelAttack();
		}
	}

	if (AttackComponent) AttackComponent->SetCombatTarget(AttackTarget);

	if (AttackTarget)
	{
		if (MoveComponent)
		{
			MoveComponent->StopMovement();
			MoveComponent->bIsSearchAttack = bIsSearch;
		}
		return;
	}

	MoveComponent->bIsSearchAttack = bIsSearch;
	MoveComponent->SetMoveTarget(ClickLocation, TargetActor);
}
bool ABaseChampion::Server_ProcessMoveInput_Validate(FVector ClickLocation, AActor* TargetActor, bool bIsSearch)
{
	return true;
}

void ABaseChampion::PressSkill(const uint8 skilltype)
{
	if (bIsRecalling)
	{
		Server_CancelRecall();
		return;
	}

	// 사망 상태 확인
	if (StateComponent->HasStatusTag(LOLTags::State_Dead)) return;

	// [수정] 스턴이나 넉백 중일 때, 예외 허용('r')이 아니라면 스킬 차단
	if (bIsStunned || bIsKnockedBack)
	{
		if (!CanCastWhileStunned(skilltype))
		{
			return; // R스킬이 아니면 여기서 차단됨
		}
	}

	if (skilltype == 'q') {
		Skill_Q();
	}
	if (skilltype == 'w') {
		Skill_W();
	}
	if (skilltype == 'e') {
		Skill_E();
	}
	if (skilltype == 'r') {
		Skill_R();
	}
}

//스턴 로직
void ABaseChampion::ApplyStun(float Duration)
{
	bIsStunned = true;

	StopAnimMontage();

	GetWorldTimerManager().SetTimer(
		StunHandle,
		this,
		&ABaseChampion::ClearStun,
		Duration,
		false
	);
}

//스턴해제
void ABaseChampion::ClearStun()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 이동 복구

	bIsStunned = false;

	AttackComponent->ResetAttack();
}


void ABaseChampion::MoveForward(float Value)
{
	if (bIsKnockedBack) return; //에어본 중이면 함수 종료

	if (bIsStunned) return;

	AddMovementInput(GetActorForwardVector(), Value);
}


void ABaseChampion::SetIsKnockedBack(bool bInKnockback)
{
	// 실제 로직 (예: 변수 업데이트)
	bIsKnockedBack = bInKnockback;
}

void ABaseChampion::Multicast_PlayMontage_Implementation(UAnimMontage* AnimMontage, float InplayRate)
{
	if (AnimMontage) PlayAnimMontage(AnimMontage, InplayRate);
}

void ABaseChampion::Multicast_SetTargetAndPlayMontage_Implementation(UAnimMontage* AnimMontage, float InplayRate, FRotator TargetRotation)
{
	SetActorRotation(TargetRotation);

	if (AnimMontage) PlayAnimMontage(AnimMontage, InplayRate);
}

void ABaseChampion::StartRecall()
{
	if (StateComponent && StateComponent->HasStatusTag(LOLTags::State_Dead)) return;
	if (bIsStunned || bIsKnockedBack) return;

	Server_StartRecall();
}

void ABaseChampion::Server_StartRecall_Implementation()
{
	if (bIsRecalling) return;
	if (StateComponent && StateComponent->HasStatusTag(LOLTags::State_Dead)) return;
	if (bIsStunned || bIsKnockedBack) return;

	if (AttackComponent)
	{
		AttackComponent->CancelAttack();
		AttackComponent->CombatTarget = nullptr;
		AttackComponent->HitTarget = nullptr;
	}
	if (MoveComponent)
	{
		MoveComponent->StopMovement();
		MoveComponent->bIsSearchAttack = false;
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	bIsRecalling = true;
	Multicast_SetRecallEffectVisible(true);

	GetWorldTimerManager().ClearTimer(RecallTimerHandle);
	GetWorldTimerManager().SetTimer(
		RecallTimerHandle,
		this,
		&ABaseChampion::CompleteRecall,
		RecallDuration,
		false
	);
}

void ABaseChampion::CancelRecall()
{
	if (HasAuthority())
	{
		EndRecall(false);
		return;
	}

	Server_CancelRecall();
}

void ABaseChampion::Server_CancelRecall_Implementation()
{
	EndRecall(false);
}

void ABaseChampion::CompleteRecall()
{
	if (!HasAuthority() || !bIsRecalling) return;

	EndRecall(true);
}

void ABaseChampion::EndRecall(bool bTeleportHome)
{
	if (!HasAuthority() || !bIsRecalling) return;

	bIsRecalling = false;
	GetWorldTimerManager().ClearTimer(RecallTimerHandle);

	if (MoveComponent)
	{
		MoveComponent->StopMovement();
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}

	if (bTeleportHome)
	{
		TeleportTo(RecallHomeLocation, RecallHomeRotation);
	}

	Multicast_SetRecallEffectVisible(false);
}

void ABaseChampion::Multicast_SetRecallEffectVisible_Implementation(bool bVisible)
{
	if (bVisible)
	{
		if (!RecallEffectSystem || RecallEffectComponent) return;

		RecallEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			RecallEffectSystem,
			GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			true
		);
		return;
	}

	if (RecallEffectComponent)
	{
		RecallEffectComponent->Deactivate();
		RecallEffectComponent->DestroyComponent();
		RecallEffectComponent = nullptr;
	}
}

void ABaseChampion::SetIsPressA(bool toggle)
{
	bIsPressA = toggle;
}
//베인 벽꿍관련 함수
void ABaseChampion::StartKnockbackWithWallCheck(const FVector& InLaunchVelocity, float MaxKnockbackTime, float InWallStunDuration)
{
	if (!HasAuthority()) return;

	SetIsKnockedBack(true);
	KnockbackDirection = InLaunchVelocity.GetSafeNormal2D();
	PendingWallStunDuration = InWallStunDuration;

	LaunchCharacter(InLaunchVelocity, true, true);

	GetWorldTimerManager().SetTimer(
		KnockbackCheckHandle, this, &ABaseChampion::CheckKnockbackWall, 0.02f, true);

	GetWorldTimerManager().SetTimer(
		KnockbackTimeoutHandle, this, &ABaseChampion::EndKnockback, MaxKnockbackTime + 0.3f, false);
		
}

void ABaseChampion::CheckKnockbackWall()
{
	if (KnockbackDirection.IsNearlyZero()) return;

	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector Start = GetActorLocation();
	const FVector End = Start + KnockbackDirection * (Radius + 30.f);

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHit = GetWorld()->SweepMultiByObjectType(
		Hits, Start, End, GetActorQuat(),
		ObjParams,
		FCollisionShape::MakeCapsule(Radius * 0.9f, HalfHeight * 0.9f),
		Params);

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || HitActor == this)
		{
			continue;
		}

		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		const bool bIsWorldStatic = HitComponent && HitComponent->GetCollisionObjectType() == ECC_WorldStatic;
		const bool bIsBuilding = Cast<ABaseBuilding>(HitActor) != nullptr;
		const bool bIsWall = bIsBuilding || (bIsWorldStatic && (Hit.bStartPenetrating || FMath::Abs(Hit.ImpactNormal.Z) < 0.5f));

		if (!bIsWall)
		{
			continue;
		}

		GetCharacterMovement()->StopMovementImmediately();
		EndKnockback();
		Multicast_ApplyStun(PendingWallStunDuration);
		return;
	}
}
void ABaseChampion::EndKnockback()
{
	GetWorldTimerManager().ClearTimer(KnockbackCheckHandle);
	GetWorldTimerManager().ClearTimer(KnockbackTimeoutHandle);
	KnockbackDirection = FVector::ZeroVector;
	SetIsKnockedBack(false);
}

void ABaseChampion::Multicast_ApplyStun_Implementation(float Duration)
{
	ApplyStun(Duration);
}

void ABaseChampion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

//침묵
void ABaseChampion::ApplySilence(float Duration)
{
	bIsSilenced = true;

	StateComponent->AddStatusTag(LOLTags::State_Silenced);

	GetWorldTimerManager().ClearTimer(SilenceHandle);

	GetWorldTimerManager().SetTimer(
		SilenceHandle,
		this,
		&ABaseChampion::ClearSilence,
		Duration,
		false
	);
}

//침묵 해제
void ABaseChampion::ClearSilence()
{
	bIsSilenced = false;

	StateComponent->RemoveStatusTag(LOLTags::State_Silenced);
}

void ABaseChampion::Multicast_ApplySilence_Implementation(float Duration)
{
	ApplySilence(Duration);
}
