// 이동 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_MoveComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_MoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULOL_MoveComponent();

	void UpdateMovement(float DeltaTime);
	void SetMoveTarget(FVector NewLocation, AActor* TargetActor);
	void StopMovement();

	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
	bool bIsMoving = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class ABaseChampion* Owner;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
