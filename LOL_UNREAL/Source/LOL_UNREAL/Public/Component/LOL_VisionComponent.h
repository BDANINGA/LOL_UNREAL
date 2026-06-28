// 시야 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_VisionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_VisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_VisionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UPROPERTY()
	float VisionRadius = 1000.0f;
};
