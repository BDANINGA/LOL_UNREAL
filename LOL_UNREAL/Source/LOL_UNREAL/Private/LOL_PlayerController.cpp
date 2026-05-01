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

	bAutoManageActiveCameraTarget = false;

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

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	CameraAnchor = GetWorld()->SpawnActor<ACamera>(ACamera::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (CameraAnchor)
	{
		CameraAnchor->SetFollowTarget(GetPawn());
		SetViewTarget(CameraAnchor);
	}	
}

void ALOL_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FreeCameraEdgeScroll(DeltaTime);
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
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnToggleCamera);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Completed, this, &ALOL_PlayerController::OnToggleCamera);
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
void ALOL_PlayerController::OnToggleCamera()
{
	if (CameraAnchor)
	{
		bool bNewLock = !CameraAnchor->IsLocked();
		CameraAnchor->SetCameraLock(bNewLock);

		if (bNewLock)
		{
			CameraAnchor->SetFollowTarget(GetPawn());
		}
	}
}

void ALOL_PlayerController::FreeCameraEdgeScroll(float DeltaTime)
{
	if (!CameraAnchor || CameraAnchor->IsLocked()) return;

	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		FVector MoveDir = FVector::ZeroVector;
		float EdgeThreshold = 10.0f; // 가장자리 인식 범위

		// 8방향 체크 로직
		if (MouseX <= EdgeThreshold) MoveDir.X = -1; // 왼쪽
		else if (MouseX >= ViewportSizeX - EdgeThreshold) MoveDir.X = 1; // 오른쪽

		if (MouseY <= EdgeThreshold) MoveDir.Y = -1;
		else if (MouseY >= ViewportSizeY - EdgeThreshold) MoveDir.Y = 1; // 아래

		if (!MoveDir.IsZero())
		{
			MoveDir.Normalize();
			CameraAnchor->MoveAnchor(MoveDir, DeltaTime);
		}
	}
}
void ALOL_PlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (IsLocalPlayerController())
	{
		InitCameraAnchor(P);
	}
}
void ALOL_PlayerController::InitCameraAnchor(APawn* TargetPawn)
{
	if (!TargetPawn) return;

	if (CameraAnchor)
	{
		CameraAnchor->Destroy();
		CameraAnchor = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	FVector ChampionLocation = TargetPawn->GetActorLocation();
	CameraAnchor = GetWorld()->SpawnActor<ACamera>(ACamera::StaticClass(), 
		ChampionLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (CameraAnchor)
	{
		CameraAnchor->SetFollowTarget(TargetPawn);
		SetViewTarget(CameraAnchor);
	}
}

void ALOL_PlayerController::OnSkillQ()
{

	ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());

	if (MyPawn)
	{
		MyPawn->Skill_Q();
	}
	// 화면 왼쪽 위에 3초 동안 빨간색 글씨를 띄웁니다
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Q Skill Used!"));
}
void ALOL_PlayerController::OnSkillW()
{
	ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());

	if (MyPawn)
	{
		MyPawn->Skill_W();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("W Skill Used!"));
}
void ALOL_PlayerController::OnSkillE()
{
	ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());

	if (MyPawn)
	{
		MyPawn->Skill_E();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("E Skill Used!"));
}
void ALOL_PlayerController::OnSkillR()
{
	ABaseChampion* MyPawn = Cast<ABaseChampion>(GetPawn());

	if (MyPawn)
	{
		MyPawn->Skill_R();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("R Skill (Ultimate) Used!"));
}

