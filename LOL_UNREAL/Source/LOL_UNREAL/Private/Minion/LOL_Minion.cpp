#include "Minion/LOL_Minion.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ALOL_Minion::ALOL_Minion()
{
 	PrimaryActorTick.bCanEverTick = true;

    // 1. 캡슐 생성 (루트 컴포넌트)
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->InitCapsuleSize(34.f, 88.f);
    SetRootComponent(CapsuleComp);

    // 2. 메쉬 생성 및 캡슐에 부착
    MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);

    // 3. 이동 컴포넌트 생성
    // FloatingPawnMovement는 중력 계산은 단순하지만 AddMovementInput을 쓸 수 있게 해줍니다.
    MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
    MovementComp->MaxSpeed = 300.f;

    // 기본 팀 설정

}

void ALOL_Minion::BeginPlay()
{
	Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

void ALOL_Minion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALOL_Minion::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

