// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_CameraControlComponent.h"
#include "BaseChampion.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
ULOL_CameraControlComponent::ULOL_CameraControlComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULOL_CameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// GetOwner()는 이 컴포넌트를 들고 있는 액터를 반환합니다.
	OwnerChampion = Cast<ABaseChampion>(GetOwner());
	if (OwnerChampion)
	{
		// 주인에게 직접 카메라 붐을 달라고 요청합니다.
		CameraBoom = OwnerChampion->CameraBoom;
	}
}
// Called every frame
void ULOL_CameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 고정 상태가 아닐 때만 마우스 가장자리 이동 로직 실행
    if (!IsCameraLocked)
    {
        UpdateCameraPan(DeltaTime);
    }
}

void ULOL_CameraControlComponent::UpdateCameraPan(float DeltaTime)
{
    if (!OwnerChampion || !CameraBoom) return;

    APlayerController* PC = Cast<APlayerController>(OwnerChampion->GetController());
    if (!PC || !PC->IsLocalController()) return;

    float MouseX, MouseY;
    if (PC->GetMousePosition(MouseX, MouseY))
    {
        int32 SizeX, SizeY;
        PC->GetViewportSize(SizeX, SizeY);

        // 1. 입력 방향 결정 (화면 기준)
        float InputX = 0.f; // 상하
        float InputY = 0.f; // 좌우

        // else if를 쓰지 않고 각각 검사해야 대각선(8방향)이 가능합니다.
        if (MouseY <= EdgeThreshold) InputX = 1.f;          // 화면 위쪽
        if (MouseY >= SizeY - EdgeThreshold) InputX = -1.f; // 화면 아래쪽
        if (MouseX <= EdgeThreshold) InputY = -1.f;         // 화면 왼쪽
        if (MouseX >= SizeX - EdgeThreshold) InputY = 1.f;  // 화면 오른쪽

        if (InputX != 0.f || InputY != 0.f)
        {
            // 2. 카메라의 Yaw(회전) 값만 가져와서 평면 이동 방향을 계산합니다.
            // 우리 카메라는 -90도 회전되어 있으므로 이 값을 기준으로 방향을 잡아야 합니다.
            FRotator SubRotation = CameraBoom->GetRelativeRotation();
            FRotator YawRotation(0, SubRotation.Yaw, 0);

            // 카메라가 앞(Forward)이라고 생각하는 월드 방향과 오른쪽(Right) 방향 추출
            FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
            FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

            // 3. 최종 이동 벡터 계산 (8방향 합산)
            FVector MoveDirection = (Forward * InputX) + (Right * InputY);

            // 4. 위치 업데이트
            FVector NewLocation = CameraBoom->GetRelativeLocation() + (MoveDirection.GetSafeNormal() * MoveSpeed * DeltaTime);
            CameraBoom->SetRelativeLocation(NewLocation);
        }
    }
}

void ULOL_CameraControlComponent::SetCameraLock(bool bLock)
{
    IsCameraLocked = bLock;

    // Space바를 누르는 순간(bLock == true), 위치를 즉시 초기화
    if (IsCameraLocked && CameraBoom)
    {
        CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    }
}
