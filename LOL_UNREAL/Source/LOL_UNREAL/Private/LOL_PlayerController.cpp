#include "LOL_PlayerController.h"
#include "InputMappingContext.h"
#include "InputAction.h"        
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ALOL_PlayerController::ALOL_PlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Default(TEXT("/Game/Level/input/IMC_Default.IMC_Default"));
	if (IMC_Default.Succeeded())
	{
		DefaultMappingContext = IMC_Default.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_ClickMove(TEXT("/Game/Level/input/IA_ClickMove.IA_ClickMove"));
	if (IA_ClickMove.Succeeded())
	{
		ClickMoveAction = IA_ClickMove.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillQ(TEXT("/Game/Level/input/IA_SkillQ.IA_SkillQ"));
	if (IA_SkillQ.Succeeded())
	{
		SkillQAction = IA_SkillQ.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillW(TEXT("/Game/Level/input/IA_SkillW.IA_SkillW"));
	if (IA_SkillW.Succeeded())
	{
		SkillWAction = IA_SkillW.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillE(TEXT("/Game/Level/input/IA_SkillE.IA_SkillE"));
	if (IA_SkillE.Succeeded())
	{
		SkillEAction = IA_SkillE.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillR(TEXT("/Game/Level/input/IA_SkillR.IA_SkillR"));
	if (IA_SkillR.Succeeded())
	{
		SkillRAction = IA_SkillR.Object;
	}
}

void ALOL_PlayerController::BeginPlay()
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

void ALOL_PlayerController::PlayerTick(float DeltaTime)
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

void ALOL_PlayerController::Server_SetTargetLocation_Implementation(FVector NewLocation)
{
	// 서버측의 컨트롤러 목적지도 갱신 (서버에서도 Tick이 돌아가며 캐릭터를 이동시킴)
	TargetLocation = NewLocation;
	bIsMoving = true;
}

bool ALOL_PlayerController::Server_SetTargetLocation_Validate(FVector NewLocation)
{
	return true;
}

void ALOL_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 우클릭(ClickMoveAction)이 눌려있는 동안(Triggered) OnClickMove 및 qwer 스위치 함수를 실행
		EnhancedInputComponent->BindAction(ClickMoveAction, ETriggerEvent::Triggered, this, &ALOL_PlayerController::OnClickMove);
		EnhancedInputComponent->BindAction(SkillQAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(SkillWAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(SkillRAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillR);
	}
}

void ALOL_PlayerController::OnClickMove()
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

void ALOL_PlayerController::OnSkillQ()
{
	// 화면 왼쪽 위에 3초 동안 빨간색 글씨를 띄웁니다
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Q Skill Used!"));
}

void ALOL_PlayerController::OnSkillW()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("W Skill Used!"));
}

void ALOL_PlayerController::OnSkillE()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("E Skill Used!"));
}

void ALOL_PlayerController::OnSkillR()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("R Skill (Ultimate) Used!"));
}
