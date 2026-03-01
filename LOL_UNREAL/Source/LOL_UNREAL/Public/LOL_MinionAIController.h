#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "LOL_MinionAIController.generated.h"

/**
 * 
 */
UCLASS()
class LOL_UNREAL_API ALOL_MinionAIController : public AAIController
{
	GENERATED_BODY()
public:
	ALOL_MinionAIController();

protected:
	virtual void BeginPlay() override;

	// --- 인지 시스템(Perception) 영역 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	// 시야 설정 (Sight Config)
	class UAISenseConfig_Sight* SightConfig;

	// 인지 업데이트 시 호출될 함수 (타겟팅 로직의 시작점)
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// --- AI 실행 에셋 영역 ---
	UPROPERTY(EditAnywhere, Category = "AI")
	class UBehaviorTree* BTAsset;

	UPROPERTY(EditAnywhere, Category = "AI")
	class UBlackboardData* BBAsset;

	// 현재 내가 노리고 있는 대상
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	AActor* CurrentTarget;

	// 타겟을 결정하는 내부 함수
	void UpdateTarget();
};
