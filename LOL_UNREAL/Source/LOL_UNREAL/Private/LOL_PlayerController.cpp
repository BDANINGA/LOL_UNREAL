// Fill out your copyright notice in the Description page of Project Settings.
// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// 1. 이동
// 2. 공격
// ---------------------------------------------------------------------------

#include "LOL_PlayerController.h"
#include "BaseChampion.h"

#include "Net/UnrealNetwork.h"
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
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SpaceBar(TEXT("/Game/Level/input/IA_SpaceBar.IA_SpaceBar"));
	if (IA_SpaceBar.Succeeded())
	{
		SpaceBarAction = IA_SpaceBar.Object;
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
}

void ALOL_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnClickMove);
		EnhancedInputComponent->BindAction(SkillQAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(SkillWAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(SkillRAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillR);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSpaceBarPressed);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Completed, this, &ALOL_PlayerController::OnSpaceBarReleased);
	}
}
void ALOL_PlayerController::OnClickMove()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());
		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			MyPawn->ProcessMoveInput(HitResult.Location, HitResult.GetActor());
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
void ALOL_PlayerController::OnSpaceBarPressed()
{
	if (ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn()))
	{
		MyPawn->SetCameraLock(true);
	}
}

void ALOL_PlayerController::OnSpaceBarReleased()
{
	if (ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn()))
	{
		MyPawn->SetCameraLock(false);
	}
}