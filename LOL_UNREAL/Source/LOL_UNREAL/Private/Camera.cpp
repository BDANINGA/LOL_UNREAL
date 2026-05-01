// 카메라를 위해서 만든 투명 객체

#include "Camera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ACamera::ACamera()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1000.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, -90.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	bReplicates = false;
}

void ACamera::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsLocked && FollowTarget)
	{
		FVector TargetLoc = FollowTarget->GetActorLocation();
		FVector CurrentLoc = GetActorLocation();
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, FollowInterpSpeed);
		SetActorLocation(NewLoc);
	}

}
void ACamera::SetFollowTarget(AActor* Target)
{
	FollowTarget = Target;
	if (FollowTarget) SetActorLocation(FollowTarget->GetActorLocation());
}
void ACamera::MoveAnchor(FVector Direction, float DeltaTime)
{
	if (!bIsLocked)
	{
		FVector NewLocation = GetActorLocation() + (Direction * CameraMoveSpeed * DeltaTime);
		SetActorLocation(NewLocation);
	}
}

