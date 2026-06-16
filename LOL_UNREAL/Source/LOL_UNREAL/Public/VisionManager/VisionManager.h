// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisionManager.generated.h"

UCLASS()
class LOL_UNREAL_API AVisionManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AVisionManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float UpdateInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	bool bHideEnemyChampions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	bool bDrawDebugVision = false;

	void UpdateVisionForLocalPlayer();
	bool IsLocationVisible(
		const FVector& Location,
		const TArray<class ABaseChampion*>& VisionSources
	) const;
	class ABaseChampion* GetLocalChampion() const;

	float TimeSinceLastUpdate = 0.0f;
};
