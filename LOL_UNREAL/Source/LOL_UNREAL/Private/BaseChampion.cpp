// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.cpp
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// 3. 공격 대상 지정
// ----------------------------------------------------------------------------------

#include "BaseChampion.h"
#include "LOL_PlayerController.h"

#include "Net/UnrealNetwork.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ABaseChampion::ABaseChampion()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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

	// 기초 테스트 능력치
	BaseStat.AttackRange = 500.0f;
	BaseStat.AttackSpeed = 1.0f;
	bCanAttack = true; // 초기값 설정
}

// Called when the game starts or when spawned
void ABaseChampion::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called to bind functionality to input
void ABaseChampion::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ABaseChampion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseChampion, BaseStat);
}

void ABaseChampion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 로직을 계산하도록 HasAuthority()를 체크합니다. (네트워크 최적화)
	if (HasAuthority() && CombatTarget)
	{
		CheckAttackRange();
	}
}

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
	if (Distance <= BaseStat.AttackRange)
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
	if (!bCanAttack || !CombatTarget) return;

	bCanAttack = false;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("공격중!"));

	Multicast_PlayAttackMontage();

	// 공격 속도(AttackSpeed)를 초 단위 주기로 변환하여 타이머 설정
	// 예: 공속이 1.0이면 1초에 한 번, 2.0이면 0.5초에 한 번
	float AttackDelay = 1.0f / BaseStat.AttackSpeed;

	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ABaseChampion::ResetAttack, AttackDelay, false);
}

void ABaseChampion::ResetAttack()
{
	// 타이머가 끝나면 다시 공격할 수 있는 상태로 변경
	bCanAttack = true;
}

void ABaseChampion::Multicast_PlayAttackMontage_Implementation()
{
	if (AttackMontage)
	{
		// 이 코드가 이제 모든 플레이어의 화면에서 실행됩니다.
		PlayAnimMontage(AttackMontage);
	}
}