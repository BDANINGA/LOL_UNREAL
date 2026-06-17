// Fill out your copyright notice in the Description page of Project Settings.
// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// 1. 이동
// 2. 공격
// ---------------------------------------------------------------------------

#include "LOL_PlayerController.h"
#include "BaseChampion.h"
#include "Minion/BaseMinion.h"
#include "Camera.h"

#include "Widget/LOL_CursorWidget.h"

#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_UIComponent.h"

#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h" 
#include "NiagaraSystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

ALOL_PlayerController::ALOL_PlayerController()
{
	bAutoManageActiveCameraTarget = false;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Default(TEXT("/Game/Level/input/IMC_Default.IMC_Default"));
	if (IMC_Default.Succeeded()) DefaultMappingContext = IMC_Default.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_RightClick(TEXT("/Game/Level/input/IA_RightClick.IA_RightClick"));
	if (IA_RightClick.Succeeded()) RightClickAction = IA_RightClick.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_LeftClick(TEXT("/Game/Level/input/IA_LeftClick.IA_LeftClick"));
	if (IA_LeftClick.Succeeded()) LeftClickAction = IA_LeftClick.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillQ(TEXT("/Game/Level/input/IA_SkillQ.IA_SkillQ"));
	if (IA_SkillQ.Succeeded()) SkillQAction = IA_SkillQ.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillW(TEXT("/Game/Level/input/IA_SkillW.IA_SkillW"));
	if (IA_SkillW.Succeeded()) SkillWAction = IA_SkillW.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillE(TEXT("/Game/Level/input/IA_SkillE.IA_SkillE"));
	if (IA_SkillE.Succeeded()) SkillEAction = IA_SkillE.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SkillR(TEXT("/Game/Level/input/IA_SkillR.IA_SkillR"));
	if (IA_SkillR.Succeeded()) SkillRAction = IA_SkillR.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SpaceBar(TEXT("/Game/Level/input/IA_SpaceBar.IA_SpaceBar"));
	if (IA_SpaceBar.Succeeded()) SpaceBarAction = IA_SpaceBar.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> IA_AKey(TEXT("/Game/Level/input/IA_A.IA_A"));
	if (IA_AKey.Succeeded()) AKeyAction = IA_AKey.Object;

	static ConstructorHelpers::FClassFinder<ULOL_CursorWidget> CursorWidgetAsset(TEXT("/Game/UI/Cursor/Wbp_CursorWidget.Wbp_CursorWidget_C"));
	if (CursorWidgetAsset.Succeeded()) CursorWidgetClass = CursorWidgetAsset.Class;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXAsset(TEXT("/Game/UI/Cursor/Indicator/FX_ClickIndicator.FX_ClickIndicator"));
	if (FXAsset.Succeeded()) ClickFX = FXAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FXAsset1(TEXT("/Game/UI/Cursor/Indicator/FX_AttackIndicator.FX_AttackIndicator"));
	if (FXAsset1.Succeeded()) AClickFX = FXAsset1.Object;
}

void ALOL_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (CursorWidgetClass)
		{
			MyCursorWidget = CreateWidget<ULOL_CursorWidget>(this, CursorWidgetClass);
			if (MyCursorWidget)
			{
				SetMouseCursorWidget(EMouseCursor::Default, MyCursorWidget);
			}
		}

	}
	
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
void ALOL_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	MyChampion = Cast<ABaseChampion>(InPawn);
}

void ALOL_PlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	MyChampion = Cast<ABaseChampion>(GetPawn());
}
void ALOL_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController())
	{
		UpdateCursorSelection();
		FreeCameraEdgeScroll(DeltaTime);
	}

	FreeCameraEdgeScroll(DeltaTime);
}

void ALOL_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnRightClick);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnLeftClick);
		EnhancedInputComponent->BindAction(SkillQAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillQ);
		EnhancedInputComponent->BindAction(SkillWAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillW);
		EnhancedInputComponent->BindAction(SkillEAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillE);
		EnhancedInputComponent->BindAction(SkillRAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnSkillR);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnToggleCamera);
		EnhancedInputComponent->BindAction(SpaceBarAction, ETriggerEvent::Completed, this, &ALOL_PlayerController::OnToggleCamera);
		EnhancedInputComponent->BindAction(AKeyAction, ETriggerEvent::Started, this, &ALOL_PlayerController::OnAKey);
	}
}
void ALOL_PlayerController::OnRightClick()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AActor* HitActor = HitResult.GetActor();
		if (MyChampion && MyChampion->IsLocallyControlled())
		{
			MyChampion->SetIsPressA(false);
			MyChampion->MoveComponent->bIsSearchAttack = false;
			MyChampion->UIComponent->HideRangeIndicator();
			MyChampion->ProcessMoveInput(HitResult.Location, HitActor);

			if (Cast<ABaseChampion>(HitActor) == nullptr && Cast<ABaseMinion>(HitActor) == nullptr) 
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ClickFX,
					HitResult.Location + FVector(0.f, 0.f, 20.f),
					FRotator(-90.f, 0.f, 0.f)
				);
			}
		}
	}
}
void ALOL_PlayerController::OnLeftClick()
{
	if (MyChampion && MyChampion->IsLocallyControlled())
	{
		if (MyChampion->GetIsPressA())
		{
			FHitResult HitResult;
			
			if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
			{
				AActor* HitActor = HitResult.GetActor();
				MyChampion->ProcessMoveInput(HitResult.Location, HitActor);
				MyChampion->SetIsPressA(false);
				MyChampion->MoveComponent->bIsSearchAttack = true;
				MyChampion->UIComponent->HideRangeIndicator();

				if (Cast<ABaseChampion>(HitActor) == nullptr && Cast<ABaseMinion>(HitActor) == nullptr)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						AClickFX,
						HitResult.Location + FVector(0.f, 0.f, 20.f),
						FRotator(-90.f, 0.f, 0.f)
					);
				}
			}
		}
	}
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
	if (MyChampion)
	{
		MyChampion->PressSkill('q');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Q Skill Used!"));
}
void ALOL_PlayerController::OnSkillW()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('w');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("W Skill Used!"));
}
void ALOL_PlayerController::OnSkillE()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('e');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("E Skill Used!"));
}
void ALOL_PlayerController::OnSkillR()
{
	if (MyChampion)
	{
		MyChampion->PressSkill('r');
		MyChampion->SetIsPressA(false);
		MyChampion->UIComponent->HideRangeIndicator();
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("R Skill (Ultimate) Used!"));
}
void ALOL_PlayerController::OnAKey()
{
	if (MyChampion)
	{
		MyChampion->SetIsPressA(true);
		MyChampion->UIComponent->ShowRangeIndicator();
	}
}
void ALOL_PlayerController::UpdateCursorSelection()
{
	if (!IsLocalController() || !MyCursorWidget) return;
	if (!MyChampion) return;

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		AActor* TargetActor = Hit.GetActor();
		bool bIsEnemyTarget = false;
		if (TargetActor && TargetActor != MyChampion && TargetActor->FindComponentByClass<ULOL_StateComponent>())
		{
			bIsEnemyTarget = MyChampion->IsEnemyActor(TargetActor);
		}
		if (bIsEnemyTarget && MyChampion->GetIsPressA())
		{
			ChangeCursorType(TEXT("SelectEnemy"));
			return;
		}
		else if (bIsEnemyTarget)
		{
			ChangeCursorType(TEXT("Attack")); 
			return;
		}
		else if (MyChampion->GetIsPressA())
		{
			ChangeCursorType(TEXT("Select")); 
			return;
		}
	}

	ChangeCursorType(TEXT("Normal")); 
}
void ALOL_PlayerController::ChangeCursorType(FString NewStateName)
{
	if (MyCursorWidget && LastCursorState != NewStateName)
	{
		MyCursorWidget->SwitchCursorState(NewStateName);
		LastCursorState = NewStateName;
	}
}
