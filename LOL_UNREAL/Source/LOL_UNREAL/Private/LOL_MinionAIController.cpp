#include "LOL_MinionAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

ALOL_MinionAIController::ALOL_MinionAIController()
{
	// 1. Perception 컴포넌트 생성
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

	// 2. 시야(Sight) 설정 객체 생성
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 시야 세부 설정 (미니언 사거리에 맞춰 조절)
	SightConfig->SightRadius = 800.f;        // 인지 거리
	SightConfig->LoseSightRadius = 1000.f;   // 놓치는 거리
	SightConfig->PeripheralVisionAngleDegrees = 90.f; // 시야각

	// 팀 감지 설정 (적군, 아군, 중립 모두 감지하도록)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// 인지 컴포넌트에 시야 설정 등록
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// 3. 인지 이벤트 바인딩 (누군가 시야에 들어오면 함수 실행)
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ALOL_MinionAIController::OnTargetPerceptionUpdated);
}

void ALOL_MinionAIController::BeginPlay()
{
	Super::BeginPlay();

	// 나중에 BTAsset과 BBAsset을 블루프린트에서 할당한 뒤 실행할 코드
	if (BTAsset)
	{
		RunBehaviorTree(BTAsset);
	}
}

void ALOL_MinionAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 타겟팅 우선순위(Target Selection) 로직

	UpdateTarget(); // 인지 정보가 바뀔 때마다 타겟팅 다시 계산
}

// 여기 구조는 다시 공부해야함
void ALOL_MinionAIController::UpdateTarget()
{
	TArray<AActor*> PerceivedActors;
	AIPerceptionComp->GetCurrentlyPerceivedActors(UAISenseConfig_Sight::StaticClass(), PerceivedActors);

	AActor* BestTarget = nullptr;
	int32 HighestPriority = -1;

	for (AActor* Actor : PerceivedActors)
	{
		// TODO: 팀 인터페이스를 이용해 적군인지 확인 (IsEnemy? 로직)

		// 우선순위 점수 계산 예시 (간단하게)
		int32 Priority = 0;
		// if (Actor->IsA(AChampion::StaticClass())) Priority = 3;  // 챔피언 우선
		// else if (Actor->IsA(AMLOL_Minion::StaticClass())) Priority = 2; // 그다음 미니언

		if (Priority > HighestPriority)
		{
			HighestPriority = Priority;
			BestTarget = Actor;
		}
	}

	// 최종 결정된 타겟을 블랙보드에 업데이트
	CurrentTarget = BestTarget;
	if (GetBlackboardComponent())
	{
		GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), CurrentTarget);
	}
}