// 시야 컴포넌트
#include "Component/LOL_VisionComponent.h"
#include "VisionManager/VisionManager.h"
#include "Kismet/GameplayStatics.h"

ULOL_VisionComponent::ULOL_VisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void ULOL_VisionComponent::BeginPlay()
{
	Super::BeginPlay();
	AVisionManager* Manager = Cast<AVisionManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AVisionManager::StaticClass()));
	if (Manager)
	{
		Manager->RegisterVisionComponent(this);
	}
}

void ULOL_VisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AVisionManager* Manager = Cast<AVisionManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AVisionManager::StaticClass()));
	if (Manager)
	{
		Manager->UnregisterVisionComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}
