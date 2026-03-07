#include "LOLPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ALOLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	

	// . 목적지가 있다면 이동 처리
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

void ALOLPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 우클릭(ClickMoveAction)이 눌려있는 동안(Triggered) OnClickMove 및 qwer 스위치 함수를 실행
		EnhancedInputComponent->BindAction(ClickMoveAction, ETriggerEvent::Triggered, this, &ALOLPlayerController::OnClickMove);
		EnhancedInputComponent->BindAction(SkillQAction, ETriggerEvent::Started, this, &ALOLPlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(SkillWAction, ETriggerEvent::Started, this, &ALOLPlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ALOLPlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(SkillRAction, ETriggerEvent::Started, this, &ALOLPlayerController::OnSkillR);
	}
}

void ALOLPlayerController::OnClickMove()
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

void ALOLPlayerController::OnSkillQ()
{
	// 화면 왼쪽 위에 3초 동안 빨간색 글씨를 띄웁니다
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Q Skill Used!"));
}

void ALOLPlayerController::OnSkillW()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("W Skill Used!"));
}

void ALOLPlayerController::OnSkillE()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("E Skill Used!"));
}

void ALOLPlayerController::OnSkillR()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("R Skill (Ultimate) Used!"));
}
