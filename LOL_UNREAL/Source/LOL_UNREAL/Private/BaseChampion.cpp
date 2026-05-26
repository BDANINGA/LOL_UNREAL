// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.cpp
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// 3. 공격 대상 지정
// ----------------------------------------------------------------------------------
#include "BaseChampion.h"
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h" 
#include "NiagaraSystem.h"
#include "Engine/DamageEvents.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/Champion_SkillComponent.h"
#include "DrawDebugHelpers.h"

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

	// Skill
	SkillComponent = CreateDefaultSubobject<UChampion_SkillComponent>(TEXT("SkillComponent"));

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXAsset(TEXT("/Game/UI/NS_ClickIndicator.NS_ClickIndicator"));
	if (FXAsset.Succeeded()) ClickFX = FXAsset.Object;

	static ConstructorHelpers::FObjectFinder<UDataTable> ResourceDataAssetTable(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionResource.Data_ChampionResource"));
	if (ResourceDataAssetTable.Succeeded()) DataTable = ResourceDataAssetTable.Object;

	// 캡슐 컴포넌트의 콜리전 설정
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
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
		Movement->bCanWalkOffLedges = false;
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

	if (StatComponent) StatComponent->InitializeStat();

	if (SkillComponent) SkillComponent->InitializeSkills();

	// UI 설정
	if (StatComponent && UIComponent)
	{
		AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseChampion::OnEnemyEnterRange);
		AttackRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseChampion::OnEnemyLeaveRange);
		AttackRangeSphere->SetSphereRadius(StatComponent->GetStat().AttackRange);

		StatComponent->OnHpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateHpFromStat);
		StatComponent->OnMpChanged.AddUObject(UIComponent, &ULOL_UIComponent::UpdateMpFromStat);
		StatComponent->OnHpZero.AddUObject(LifeCycleComponent, &ULOL_LifeCycleComponent::Server_HandleDeath);
		
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
					MyHUD->UpdateAll_Images(this);
					MyHUD->UpdateStat(StatComponent->GetStat());
					MyHUD->UpdateHP(StatComponent->GetCurrentHP());
					MyHUD->UpdateMP(StatComponent->GetCurrentMP());
				}
			}
		}
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

	if (HasStatusTag(LOLTags::State_Dead)) return;

	if (CombatTarget)
	{
		AttackComponent->UpdateAttackLogic();
	}
	else {
		MoveComponent->UpdateMovement(DeltaTime); // 분리된 로직 호출
	}
}
float ABaseChampion::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
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
		ActualDamage = StatComponent->ApplyDamage(ActualDamage, Type);
	}

	return ActualDamage;
}
void ABaseChampion::OnEnemyEnterRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	ABaseChampion* Enemy = Cast<ABaseChampion>(OtherActor);

	// 팀 조건 추가해야함.
	if (Enemy && Enemy != this && !Enemy->HasStatusTag(LOLTags::State_Dead))
	{
		EnemiesInRange.AddUnique(Enemy);
	}
}
void ABaseChampion::OnEnemyLeaveRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	ABaseChampion* Enemy = Cast<ABaseChampion>(OtherActor);

	if (Enemy && EnemiesInRange.Contains(Enemy))
	{
		EnemiesInRange.Remove(Enemy);
	}
}
void ABaseChampion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseChampion, CombatTarget);
	DOREPLIFETIME(ABaseChampion, HitTarget);
	DOREPLIFETIME(ABaseChampion, StatusTags);
}

void ABaseChampion::Server_ExecuteAttackHit_Implementation()
{
	if (AttackComponent)
	{
		// 이 코드는 100% 서버에서만 실행되므로 안심하고 데미지를 줍니다.
		AttackComponent->ExecuteAttackHit();
	}
}
void ABaseChampion::ProcessMoveInput(FVector ClickLocation, AActor* TargetActor)
{
	ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
	if (TargetChampion == this)
	{
		TargetChampion = nullptr;
		TargetActor = nullptr;
	}
	if (ClickFX && TargetChampion == nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ClickFX,
			ClickLocation + FVector(0.f, 0.f, 20.f), // 바닥에 살짝 띄움
			FRotator(-90.f, 0.f, 0.f)
		);
	}
	Server_ProcessMoveInput(ClickLocation, TargetActor, bIsPressA);
}
void ABaseChampion::Server_ProcessMoveInput_Implementation(FVector ClickLocation, AActor* TargetActor, bool bIsSearch)
{
	if (bIsKnockedBack) return;
	if (HasStatusTag(LOLTags::State_Dead)) return;
	if (bIsStunned) return;

	MoveComponent->bIsSearchAttack = bIsSearch;

	ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
	if (!TargetChampion && TargetChampion == this)
	{
		TargetChampion = nullptr;
		TargetActor = nullptr;
	}

	if (AttackComponent && !AttackComponent->CanAttack())
	{
		if (TargetChampion == nullptr || TargetChampion != CombatTarget)
		{
			AttackComponent->CancelAttack();
		}
	}

	CombatTarget = TargetChampion;

	MoveComponent->bIsSearchAttack = bIsSearch;
	MoveComponent->SetMoveTarget(ClickLocation, TargetChampion);
}
bool ABaseChampion::Server_ProcessMoveInput_Validate(FVector ClickLocation, AActor* TargetActor, bool bIsSearch)
{
	return true;
}

void ABaseChampion::PressSkill(const uint8 skilltype)
{
	// 사망 상태 확인
	if (HasStatusTag(LOLTags::State_Dead)) return;

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

inline void ABaseChampion::SetIsPressA(bool toggle)
{
	bIsPressA = toggle;
}
void ABaseChampion::AddStatusTag(FGameplayTag Tag)
{
	if (HasAuthority()) { StatusTags.AddTag(Tag); }
}

void ABaseChampion::RemoveStatusTag(FGameplayTag Tag)
{
	if (HasAuthority()) { StatusTags.RemoveTag(Tag); }
}

bool ABaseChampion::HasStatusTag(FGameplayTag Tag) const
{
	return StatusTags.HasTag(Tag);
}

void ABaseChampion::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = StatusTags;
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
	const FVector End = Start + KnockbackDirection * (Radius + 30.f); // 거리 살짝 늘림

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// ★ 트레이스 채널 대신 '오브젝트 타입(WorldStatic)' 질의 → 벽/지형 확실히 감지
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const bool bHit = GetWorld()->SweepSingleByObjectType(
		Hit, Start, End, GetActorQuat(),
		ObjParams,
		FCollisionShape::MakeCapsule(Radius * 0.9f, HalfHeight * 0.9f),
		Params);

	// 디버그: 초록=벽 감지, 빨강=못 잡음
	DrawDebugCapsule(GetWorld(), End, HalfHeight * 0.9f, Radius * 0.9f,
		GetActorQuat(), bHit ? FColor::Green : FColor::Red, false, 0.1f);

	if (bHit)
	{
		// 바닥/경사면 제외(수평 노멀만 벽으로 인정), 단 벽에 파고든 경우도 허용
		const bool bIsWall = Hit.bStartPenetrating || FMath::Abs(Hit.ImpactNormal.Z) < 0.5f;
		if (bIsWall)
		{
			GetCharacterMovement()->StopMovementImmediately();
			EndKnockback();
			Multicast_ApplyStun(PendingWallStunDuration);
		}
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
