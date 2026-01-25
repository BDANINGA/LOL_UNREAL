#include "LOLPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "GameFramework/Character.h"

ALOLPlayerController::ALOLPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs; 
}

void ALOLPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALOLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 1. 우클릭 시 목적지 갱신
	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		FHitResult HitResult;
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
		if (HitResult.bBlockingHit)
		{
			TargetLocation = HitResult.Location;
			bIsMoving = true;

			Server_SetTargetLocation(HitResult.Location);
		}
	}

	// 2. 목적지가 있다면 이동 처리
	if (bIsMoving)
	{
		APawn* const MyPawn = GetPawn();
		if (MyPawn)
		{
			FVector CurrentLocation = MyPawn->GetActorLocation();
			FVector Direction = TargetLocation - CurrentLocation;
			Direction.Z = 0.f;

			float Distance = Direction.Size();

			if (Distance <= 10.f)
			{
				bIsMoving = false;
			}
			else
			{
				MyPawn->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
			}
		}
	}
}

void ALOLPlayerController::Server_SetTargetLocation_Implementation(FVector NewLocation)
{
	// 서버측의 컨트롤러 목적지도 갱신 (서버에서도 Tick이 돌아가며 캐릭터를 이동시킴)
	TargetLocation = NewLocation;
	bIsMoving = true;
}

bool ALOLPlayerController::Server_SetTargetLocation_Validate(FVector NewLocation)
{
	return true;
}
