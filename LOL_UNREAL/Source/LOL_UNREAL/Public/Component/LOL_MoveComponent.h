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

	UFUNCTION(Client, Reliable)
	void Client_SetNavigationPath(const TArray<FVector>& NewPathPoints);

	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated)
	bool bIsSearchAttack = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	class APawn* OwnerPawn;

	bool bUsingDirectMovement = false;
	TArray<FVector> LocalNavigationPath;
	int32 CurrentNavigationPathIndex = 0;
	TWeakObjectPtr<AActor> MovementTargetActor;

	void SetLocalNavigationPath(const TArray<FVector>& NewPathPoints);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
