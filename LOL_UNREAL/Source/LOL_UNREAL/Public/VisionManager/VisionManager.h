#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisionManager.generated.h"

class ULOL_VisionComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UWidgetComponent;

UCLASS()
class AVisionManager : public AActor
{
	GENERATED_BODY()

public:
	AVisionManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	void RegisterVisionComponent(ULOL_VisionComponent* Component);
	void UnregisterVisionComponent(ULOL_VisionComponent* Component);
	void RegisterActor(AActor* Actor);
	void UnregisterActor(AActor* Actor);

	void UpdateFoW();

	// Shared by local rendering and server-side targeting; never depends on local hidden state.
	bool CanTeamSeeActor(const AActor* Viewer, const AActor* Target) const;
	bool CanSourceSeeActor(const ULOL_VisionComponent* Source, const AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Vision|Bush")
	void RebuildBushRegions();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Bush")
	FString BushMeshName = TEXT("stylized_bush");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Bush", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float BushFootprintScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Bush", meta = (ClampMin = "0.0"))
	float BushHeight = 300.f;

	// Also permits explicitly authored regions and deterministic gameplay tests.
	void SetBushRegions(const TArray<FBox>& Regions);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Render")
	UTextureRenderTarget2D* FoWRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Render")
	UMaterialInterface* VisionBrushMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
	FVector2D MapMinBounds = FVector2D(-5000.0f, -5000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
	FVector2D MapMaxBounds = FVector2D(5000.0f, 5000.0f);

private:
	TArray<FBox> BushRegions;
	TArray<int32> BushGroups;
	TMap<TWeakObjectPtr<UWidgetComponent>, bool> HiddenWidgetVisibility;
	UPROPERTY()
	TArray<ULOL_VisionComponent*> BlueVisionComponents;
	UPROPERTY()
	TArray<ULOL_VisionComponent*> RedVisionComponents;

	UPROPERTY()
	TArray<AActor*> BlueActors;
	UPROPERTY()
	TArray<AActor*> RedActors;

	UPROPERTY()
	UMaterialInstanceDynamic* VisionBrushMID;
};
