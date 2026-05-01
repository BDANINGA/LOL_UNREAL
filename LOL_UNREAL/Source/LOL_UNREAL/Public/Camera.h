// 카메라를 위해서 만든 투명 객체

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera.generated.h"

UCLASS()
class LOL_UNREAL_API ACamera : public AActor
{
	GENERATED_BODY()
	
public:	
	ACamera();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// 카메라 컴포넌트들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera;

	// 제어 변수
	void SetFollowTarget(AActor* Target);
	void SetCameraLock(bool bLock) { bIsLocked = bLock; }
	bool IsLocked() const { return bIsLocked; }
	void MoveAnchor(FVector Direction, float DeltaTime);

private:
	UPROPERTY()
	AActor* FollowTarget;

	bool bIsLocked = false;
	float CameraMoveSpeed = 2500.f;
	float FollowInterpSpeed = 20.f; // 카메라가 따라가는 부드러움 정도
};
