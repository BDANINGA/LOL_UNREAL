// Fill out your copyright notice in the Description page of Project Settings.
// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// 1. 이동
// 2. 공격
// ---------------------------------------------------------------------------

#include "LOL_PlayerController.h"
#include "BaseChampion.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"


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

// 서버 - 이동 로직
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
// 서버 - 타겟 설정 로직
void ALOL_PlayerController::Server_SetCombatTarget_Implementation(AActor* Target)
{
	ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());
	if (MyPawn)
	{
		MyPawn->SetCombatTarget(Target); 
	}
}
bool ALOL_PlayerController::Server_SetCombatTarget_Validate(AActor* Target)
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

	// 마우스 아래에 있는 것이 무엇인지 검사
	bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (bHit && HitResult.bBlockingHit)
	{
		AActor* HitActor = HitResult.GetActor();
		UE_LOG(LogTemp, Log, TEXT("Clicked Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));

		ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());

		if (MyPawn)
		{
			// 클릭한 것이 챔피언 (나 자신이 아닐 때)
			if (HitActor && HitActor->IsA(ABaseChampion::StaticClass()) && HitActor != MyPawn)
			{
				// 공격 타겟으로 지정 (서버에 요청)
				Server_SetCombatTarget(HitActor);

				// 공격 중일 때는 직접적인 좌표 이동은 잠시 멈춤
				bIsMoving = false;
			}
			
			// 바닥이나 일반 물체라면 이동 처리
			else
			{
				TargetLocation = HitResult.Location;
				bIsMoving = true;

				// 서버에게 타겟 해제 및 이동 좌표 전달
				Server_SetCombatTarget(nullptr);
				Server_SetTargetLocation(HitResult.Location);
			}
		}
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
