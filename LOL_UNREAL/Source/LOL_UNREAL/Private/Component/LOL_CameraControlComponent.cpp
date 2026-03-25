// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LOL_CameraControlComponent.h"
#include "BaseChampion.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"

ULOL_CameraControlComponent::ULOL_CameraControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}
void ULOL_CameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerChampion = Cast<ABaseChampion>(GetOwner());
	if (OwnerChampion)
	{
		// 주인에게 직접 카메라 붐을 달라고 요청
		CameraBoom = OwnerChampion->CameraBoom;
	}
}
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

        // 1. 순수 입력 벡터 초기화 (화면 기준)
        FVector RawDirection = FVector::ZeroVector;

        // 독립적인 if문으로 8방향 판정
        if (MouseY <= EdgeThreshold) RawDirection.X = 1.f;          // 화면 위
        if (MouseY >= SizeY - EdgeThreshold) RawDirection.X = -1.f; // 화면 아래
        if (MouseX <= EdgeThreshold) RawDirection.Y = -1.f;         // 화면 왼쪽
        if (MouseX >= SizeX - EdgeThreshold) RawDirection.Y = 1.f;  // 화면 오른쪽

        if (!RawDirection.IsNearlyZero())
        {
            // 1. 현재 카메라 붐의 월드 회전값을 가져옵니다. (-90도 상태 포함)
            FRotator CameraRotation = CameraBoom->GetComponentRotation();

            // 2. 수평 이동을 위해 Pitch, Roll은 버리고 Yaw만 추출합니다.
            FRotator YawRotation(0.f, CameraRotation.Yaw, 0.f);

            // 3. [중요] 화면의 '위' 방향(RawDirection.X)은 카메라가 보는 '앞' 방향입니다.
            // [중요] 화면의 '오른쪽' 방향(RawDirection.Y)은 카메라가 보는 '오른쪽' 방향입니다.
            FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
            FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

            // 4. 입력값과 실제 벡터를 결합합니다.
            // 마우스 위(X=1) -> 카메라 정면(Forward)
            // 마우스 오른쪽(Y=1) -> 카메라 오른쪽(Right)
            FVector MoveDirection = (ForwardVector * RawDirection.X) + (RightVector * RawDirection.Y);
            MoveDirection = MoveDirection.GetSafeNormal();

            // 5. 이동 적용
            FVector NewLocation = CameraBoom->GetRelativeLocation() + (MoveDirection * MoveSpeed * DeltaTime);
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

void ULOL_CameraControlComponent::HandleCameraLockInput(bool bPressed)
{
    IsCameraLocked = bPressed;

    if (IsCameraLocked)
    {
        ResetCameraToOwner();
    }
}

void ULOL_CameraControlComponent::ResetCameraToOwner()
{
    if (CameraBoom)
    {
        // 챔피언 위치로 즉시 이동 (상대 좌표 0)
        CameraBoom->SetRelativeLocation(FVector::ZeroVector);
    }
}
