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
	CameraBoom->SetupAttachment(RootComponent); // ĳ���� ���뿡 ���̱�
	CameraBoom->TargetArmLength = 1200.0f; // ī�޶� �Ÿ� (��ó�� �ָ�)
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // 60�� ������ �����ٺ���
	CameraBoom->bDoCollisionTest = false; // ���� ��Ƶ� ���ε��� �ʰ� (�� ��Ÿ��)

	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // ����� ���� ���̱�
	FollowCamera->bUsePawnControlRotation = false; // ī�޶�� ���� ȸ������ ����

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f); // ȸ�� �ӵ�
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bConstrainToPlane = true; // ���� �� �پ �ٴϰ�
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	bReplicates = true;
	ACharacter::SetReplicateMovement(true); // 위치와 회전을 모두 복제하도록 설정

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

