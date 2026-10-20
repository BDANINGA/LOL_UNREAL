#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GragasRBarrelVisual.generated.h"

// Cosmetic flight on each multicast recipient; damage remains on the champion/server.
UCLASS()
class LOL_UNREAL_API AGragasRBarrelVisual : public AActor
{
	GENERATED_BODY()

public:
	AGragasRBarrelVisual();
	void Initialize(class UStaticMesh* Mesh, class UMaterialInterface* Material,
		const FVector& Start, const FVector& End, float Duration, float VisualRadius);
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateFlight(float Alpha);

	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> BarrelMesh;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	FVector MeshCenter = FVector::ZeroVector;
	double StartTime = 0.0;
	float TravelDuration = 1.0f;
	float ArcHeight = 200.0f;
};
