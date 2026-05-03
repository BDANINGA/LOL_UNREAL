// 자신이 플레이하고 있는 챔피언을 조작하기 위한 컨트롤러입니다.
// ---------------------------------------------------------------------------
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Camera.h"
#include "LOL_PlayerController.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALOL_PlayerController();

	void OnClickMove();
	void OnSkillQ();
	void OnSkillW();
	void OnSkillE();
	void OnSkillR();

	void OnToggleCamera();
	void FreeCameraEdgeScroll(float DeltaTime);
	virtual void AcknowledgePossession(APawn* P) override;
	void InitCameraAnchor(APawn* TargetPawn);

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class ULOL_CursorWidget> CursorWidgetClass;

	void ChangeCursorType(FString StateName);

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ClickMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillQAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillWAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillEAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SkillRAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SpaceBarAction;

private:
	UPROPERTY()
	ACamera* CameraAnchor;

	UPROPERTY()
	class ULOL_CursorWidget* MyCursorWidget;
};
