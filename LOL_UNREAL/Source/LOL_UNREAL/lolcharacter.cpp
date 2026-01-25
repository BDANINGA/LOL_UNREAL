// Fill out your copyright notice in the Description page of Project Settings.


#include "lolcharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
Alolcharacter::Alolcharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent); // 캐릭터 몸통에 붙이기
	CameraBoom->TargetArmLength = 1200.0f; // 카메라 거리 (롤처럼 멀리)
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // 60도 각도로 내려다보기
	CameraBoom->bDoCollisionTest = false; // 벽에 닿아도 줌인되지 않게 (롤 스타일)

	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // 막대기 끝에 붙이기
	FollowCamera->bUsePawnControlRotation = false; // 카메라는 절대 회전하지 않음

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f); // 회전 속도
	GetCharacterMovement()->bConstrainToPlane = true; // 땅에 딱 붙어서 다니게
	GetCharacterMovement()->bSnapToPlaneAtStart = true;



}

// Called when the game starts or when spawned
void Alolcharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void Alolcharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void Alolcharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

