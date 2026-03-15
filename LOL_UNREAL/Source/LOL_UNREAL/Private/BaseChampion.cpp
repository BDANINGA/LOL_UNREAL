// Fill out your copyright notice in the Description page of Project Settings.
// BaseChampion.cpp
// 챔피언의 기본 설정
// 1. 카메라 설정
// 2. 기본 능력치
// 3. 공격 대상 지정
// ----------------------------------------------------------------------------------

#include "BaseChampion.h"
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

	// 1. 캡슐 컴포넌트의 콜리전 설정
	// 기본적으로 Character는 GetCapsuleComponent()를 가지고 있습니다.
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 콜리전 프리셋을 Pawn으로 설정
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// Line Trace(Visibility 채널)를 Block 하도록 설정해야 마우스 클릭이 인식됩니다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 2. 테스트를 위한 기본 사거리 설정
	// 이 값이 0이면 "Attacking..." 로그가 절대 뜨지 않습니다.
	BaseStat.AttackRange = 500.0f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f; 
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); 
	CameraBoom->bDoCollisionTest = false;

	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f); 
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	bReplicates = true;
	ACharacter::SetReplicateMovement(true); // 위치와 회전을 모두 복제하도록 설정

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
		GetCharacterMovement()->StopMovementImmediately();

		UE_LOG(LogTemp, Warning, TEXT("Target in Range! Attacking..."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Target too far. Approaching..."));
	}
}