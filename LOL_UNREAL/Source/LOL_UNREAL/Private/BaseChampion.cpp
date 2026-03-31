// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.cpp
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// 3. 공격 대상 지정
// ----------------------------------------------------------------------------------

#include "BaseChampion.h"
#include "LOL_PlayerController.h"
#include "LOL_GameModeBase.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_CameraControlComponent.h"
#include "LOL_ChampionHpBarWidget.h"

// Sets default values
ABaseChampion::ABaseChampion()
{
	//Widget Component
	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Game/UI/HPbar.HPbar_C"));
	if (HpBarWidgetRef.Class) {
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		HpBar->SetWidgetSpace(EWidgetSpace::Screen);
		HpBar->SetDrawSize(FVector2D(150.0f, 20.0f));
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	//Widget Component(MP)
	MpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("MpWidget"));
	MpBar->SetupAttachment(GetMesh());
	MpBar->SetRelativeLocation(FVector(0.0f, 0.0f, 275.0f));
	static ConstructorHelpers::FClassFinder<UUserWidget> MpBarWidgetRef(TEXT("/Game/UI/MPbar.MPbar_C"));
	if (MpBarWidgetRef.Class) {
		MpBar->SetWidgetClass(MpBarWidgetRef.Class);
		MpBar->SetWidgetSpace(EWidgetSpace::Screen);
		MpBar->SetDrawSize(FVector2D(150.0f, 10.0f));
		MpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// Stat
	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));

	// Camera
	CameraControlComponent = CreateDefaultSubobject<ULOL_CameraControlComponent>(TEXT("CameraControlComponent"));

	PrimaryActorTick.bCanEverTick = true;

	// 캡슐 컴포넌트의 콜리전 설정
	// 1. 콜리전 프리셋을 Pawn으로 설정
	// 2. Line Trace를 Block 하도록 설정해야 마우스 클릭이 인식.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 캐릭터가 컨트롤러의 회전값을 직접 상속받지 않도록 확실히 차단
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 이동 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// 카메라 시스템
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f; 
	CameraBoom->SetRelativeRotation(FRotator(-60.f, -90.f, 0.f)); 
	CameraBoom->bDoCollisionTest = false;

	// 카메라 고정
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 

	// 위치와 회전을 모두 복제하도록 설정
	bReplicates = true;
	ACharacter::SetReplicateMovement(true); 
}
void ABaseChampion::BeginPlay()
{
	Super::BeginPlay();
	if (StatComponent)
	{
		// 스탯 컴포넌트의 죽음 이벤트에 나의 OnDeath 함수를 바인딩
		StatComponent->OnHpZero.AddUObject(this, &ABaseChampion::Server_HandleDeath);
	}
	if (HpBar && StatComponent)
	{
		ULOL_ChampionHpBarWidget* HpWidget = Cast<ULOL_ChampionHpBarWidget>(HpBar->GetUserWidgetObject());
		if (HpWidget)
		{
			// 최대 체력 설정
			HpWidget->SetMaxHp(StatComponent->GetStat().MaxHP);

			// StatComponent의 HP가 변할 때마다 위젯의 UpdateHpBar를 호출하도록 연결(Bind)합니다.
			StatComponent->OnHpChanged.AddUObject(HpWidget, &ULOL_ChampionHpBarWidget::UpdateHpBar);

			// 초기 HP 상태를 한 번 반영.
			HpWidget->UpdateHpBar(StatComponent->GetStat().CurrentHP);
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

	// 서버에서만 로직을 계산하도록 HasAuthority()를 체크
	if (HasAuthority() && CombatTarget)
	{
		CheckAttackRange();
	}
}

// 공격 관련 함수
void ABaseChampion::SetCombatTarget(AActor* Target)
{
	// 공격 대상을 저장합니다.
	CombatTarget = Target;
}
void ABaseChampion::CheckAttackRange()
{
	if (CombatTarget == nullptr) return;

	// 거리 계산
	float Distance = GetDistanceTo(CombatTarget);

	// 사거리 비교
	if (StatComponent && Distance <= StatComponent->GetStat().AttackRange)
	{
		ALOL_PlayerController* PC = Cast<ALOL_PlayerController>(GetController());
		if (PC)
		{
			PC->SetIsMoving(false);
		}
		GetCharacterMovement()->StopMovementImmediately();

		if (bCanAttack)
		{
			StartAttack();
		}
	}
	else
	{
		ALOL_PlayerController* PC = Cast<ALOL_PlayerController>(GetController());
		if (PC)
		{
			PC->SetIsMoving(true);
			PC->Server_SetTargetLocation(CombatTarget->GetActorLocation());
		}
	}
}
void ABaseChampion::StartAttack()
{
	if (!bCanAttack || !CombatTarget || !StatComponent) return;

	bCanAttack = false;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("공격중!"));

	Multicast_PlayAttackMontage();

	// 서버에서 데미지 계산
	if (HasAuthority())
	{
		float DamageToApply = StatComponent->GetStat().AttackDamage;

		// 상대방에게 데미지를 전달합니다. (언리얼 표준 함수 호출)
		UGameplayStatics::ApplyDamage(
			CombatTarget,
			DamageToApply,
			GetController(),
			this,
			nullptr
		);
	}

	// 공격 속도(AttackSpeed)를 초 단위 주기로 변환하여 타이머 설정
	float AttackDelay = 1.0f / StatComponent->GetStat().AttackSpeed;

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ABaseChampion::ResetAttack, AttackDelay, false);
}
void ABaseChampion::ResetAttack()
{
	// 타이머가 끝나면 다시 공격할 수 있는 상태로 변경
	bCanAttack = true;
}
float ABaseChampion::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (StatComponent)
	{
		// 컴포넌트에게 데미지 계산을 맡깁니다.
		ActualDamage = StatComponent->ApplyDamage(ActualDamage);
	}

	return ActualDamage;
}

void ABaseChampion::Multicast_PlayAttackMontage_Implementation()
{
	if (AttackMontage)
	{
		// 이 코드가 이제 모든 플레이어의 화면에서 실행됩니다.
		PlayAnimMontage(AttackMontage);
	}
}
void ABaseChampion::Multicast_PlayDeathMontage_Implementation()
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

void ABaseChampion::Server_HandleDeath()
{
	if (bIsDead) return;

	// 서버에서 먼저 상태를 바꾸고
	bIsDead = true;

	// 모든 클라이언트에게 알림
	Multicast_OnDeath();

	// 서버에서 게임모드에게 부활 요청
	if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
	{
		// 사용자님의 게임모드 클래스로 캐스팅하여 호출
		ALOL_GameModeBase* LOLGM = Cast<ALOL_GameModeBase>(GM);
		if (LOLGM)
		{
			LOLGM->RequestRespawn(this);
		}
	}
}

void ABaseChampion::Multicast_OnDeath_Implementation()
{
	// 이 함수 안의 내용은 이제 모든 플레이어의 PC에서 실행됩니다.
	OnDeath();
}

void ABaseChampion::OnDeath()
{
	// 1. 애니메이션 재생
	Multicast_PlayDeathMontage();

	// 2. 조작 금지
	GetCharacterMovement()->DisableMovement(); // 이동 정지

	// 3. 충돌 제거 (시체가 방해되지 않도록)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. UI 숨기기
	if (HpBar && MpBar)
	{
		HpBar->SetVisibility(false);
		MpBar->SetVisibility(false);
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, TEXT("챔피언 처치됨!"));
}

void ABaseChampion::Respawn()
{
	// 1. 상태 초기화
	bIsDead = false;
	if (StatComponent) StatComponent->SetHp(StatComponent->GetStat().MaxHP);
	if (StatComponent) StatComponent->SetMp(StatComponent->GetStat().MaxMP);

	// 2. 위치 이동 (본진 좌표로)
	SetActorLocation(FVector(0, 0, 100)); // 실제로는 StartSpot 좌표 사용

	// 3. 시각적 부활 처리
	Multicast_OnRespawn();
}

void ABaseChampion::Multicast_OnRespawn_Implementation()
{
	// 충돌 다시 켜기
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Block);

	// UI 다시 보이기
	if (HpBar) HpBar->SetVisibility(true);
	if (MpBar) MpBar->SetVisibility(true);

	// 애니메이션 초기화 (Idle로 돌아가기)
	PlayAnimMontage(nullptr);
}

void ABaseChampion::SetCameraLock(bool bLock)
{
	if (CameraControlComponent)
	{
		CameraControlComponent->HandleCameraLockInput(bLock);
	}
}