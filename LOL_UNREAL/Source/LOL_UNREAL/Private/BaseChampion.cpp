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
	if (FXAsset.Succeeded())
	{
		ClickFX = FXAsset.Object;
	}

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
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// 위치와 회전을 모두 복제하도록 설정
	bReplicates = true;
	ACharacter::SetReplicateMovement(true); 
}
void ABaseChampion::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent) StatComponent->InitializeStat();

	if (SkillComponent) SkillComponent->InitializeSkills();

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

	if (LifeCycleComponent->bIsDead) return;

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
	if (Enemy && Enemy != this && !Enemy->LifeCycleComponent->bIsDead)
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
void ABaseChampion::Multicast_PlayAttackMontage_Implementation(FRotator TargetRotation)
{
	SetActorRotation(TargetRotation);

	if (AttackMontage)
	{
		// 이 코드가 이제 모든 플레이어의 화면에서 실행됩니다.
		PlayAnimMontage(AttackMontage);
	}
}
void ABaseChampion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseChampion, CombatTarget);
}
void ABaseChampion::ProcessMoveInput(FVector ClickLocation, AActor* TargetActor)
{
	ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
	if (ClickFX && TargetChampion == nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ClickFX,
			ClickLocation + FVector(0.f, 0.f, 20.f), // 바닥에 살짝 띄움
			FRotator(-90.f, 0.f, 0.f)
		);
	}
	Server_ProcessMoveInput(ClickLocation, TargetActor);
}
void ABaseChampion::Server_ProcessMoveInput_Implementation(FVector ClickLocation, AActor* TargetActor)
{
	if (bIsKnockedBack) return;

	if (LifeCycleComponent->bIsDead) return;

	ABaseChampion* TargetChampion = Cast<ABaseChampion>(TargetActor);
	CombatTarget = TargetChampion;

	MoveComponent->SetMoveTarget(ClickLocation, TargetChampion);
}
bool ABaseChampion::Server_ProcessMoveInput_Validate(FVector ClickLocation, AActor* TargetActor)
{
	return true;
}

void ABaseChampion::PressSkill(char skilltype)
{
	if (skilltype == 'q') {
		if (not SkillComponent->TryCastSkill(SkillComponent->GetQ_Data(), 1)) return;
		Skill_Q();
	}
	if (skilltype == 'w') {
		if (not SkillComponent->TryCastSkill(SkillComponent->GetW_Data(), 1)) return;
		Skill_W();
	}
	if (skilltype == 'e') {
		if (not SkillComponent->TryCastSkill(SkillComponent->GetE_Data(), 1)) return;
		Skill_E();
	}
	if (skilltype == 'r') {
		if (not SkillComponent->TryCastSkill(SkillComponent->GetR_Data(), 1)) return;
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

inline void ABaseChampion::SetIsPressA(bool toggle)
{
	bIsPressA = toggle;
}
