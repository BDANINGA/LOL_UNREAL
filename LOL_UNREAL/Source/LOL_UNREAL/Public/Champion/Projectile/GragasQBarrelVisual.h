#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GragasQBarrelVisual.generated.h"

// Local cosmetic actor: the champion's multicast creates one on each client.
UCLASS()
class LOL_UNREAL_API AGragasQBarrelVisual : public AActor
{
	GENERATED_BODY()

public:
	AGragasQBarrelVisual();
	void Initialize(class UStaticMesh* Mesh, class UMaterialInterface* Material,
		const FVector& Start, const FVector& End, float Duration, float VisualRadius, float LifeTime);
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateVisual(float Alpha);

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> BarrelMesh;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	FVector MeshCenter = FVector::ZeroVector;
	float TravelDuration = 1.0f;
	float Elapsed = 0.0f;
	float RollingRadius = 40.0f;
	float MeshScale = 1.0f;
	float FallbackGroundZ = 0.0f;
};
