#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisionManager.generated.h"

class ULOL_VisionComponent;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Render")
	UTextureRenderTarget2D* FoWRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Render")
	UMaterialInterface* VisionBrushMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
	FVector2D MapMinBounds = FVector2D(-5000.0f, -5000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
	FVector2D MapMaxBounds = FVector2D(5000.0f, 5000.0f);

private:
	UPROPERTY()
	TArray<ULOL_VisionComponent*> BlueVisionComponents;
	UPROPERTY()
	TArray<ULOL_VisionComponent*> RedVisionComponents;

	UPROPERTY()
	UMaterialInstanceDynamic* VisionBrushMID;

	void UpdateFoW();
};