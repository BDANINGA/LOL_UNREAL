// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_CameraControlComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_CameraControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_CameraControlComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 카메라 고정/해제 제어
	void SetCameraLock(bool bLock);

	// 1. 마우스 좌표를 받음 
	// 2. 화면 끝인지 검사
	// 3. 카메라 붐을 이동시킨다.
	void UpdateCameraPan(float DeltaTime);

private:
	// 컴포넌트 소유자와 카메라 붐 참조
	UPROPERTY()
	class ABaseChampion* OwnerChampion;

	UPROPERTY()
	class USpringArmComponent* CameraBoom;

	bool IsCameraLocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 1500.f;

	// 마우스가 화면 끝에서 몇 픽셀 안에 들어와야 하는지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = "true"))
	float EdgeThreshold = 20.f;
		
};
