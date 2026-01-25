// Fill out your copyright notice in the Description page of Project Settings.


#include "LOLPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h" // 길찾기 이동 명령용
#include "Runtime/Engine/Classes/Components/DecalComponent.h" // 나중에 클릭 효과용
#include "GameFramework/Character.h"

ALOLPlayerController::ALOLPlayerController()
{
	bShowMouseCursor = true; // 1. 마우스 커서 보이게 설정
	DefaultMouseCursor = EMouseCursor::Crosshairs; // 2. 커서 모양 설정
}

void ALOLPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// 3. 입력 모드를 '게임+UI'로 설정 (마우스가 게임 밖으로 안 튀게)
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALOLPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 4. 마우스 오른쪽 버튼(RightMouseButton)을 누르고 있는지 검사
	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		FHitResult HitResult;
		// 마우스 커서 아래에 있는 땅바닥 위치를 찾음 (Raycasting)
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

		if (HitResult.bBlockingHit)
		{
			// 5. 찾은 위치로 캐릭터 이동 명령 내리기 (AI 기능 빌려쓰기)
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.Location);
		}
	}
}

