#pragma once

#include "CoreMinimal.h"
#include "Building/BaseBuilding.h"
#include "Building_Inhibitor.generated.h"

UCLASS()
class LOL_UNREAL_API ABuilding_Inhibitor : public ABaseBuilding
{
	GENERATED_BODY()
public:
	ABuilding_Inhibitor();
	
private:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, Category = "ABP")
	TSubclassOf<UAnimInstance> AnimBlueprint;
};
