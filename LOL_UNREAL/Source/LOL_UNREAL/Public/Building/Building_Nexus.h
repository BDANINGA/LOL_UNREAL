#pragma once

#include "CoreMinimal.h"
#include "Building/BaseBuilding.h"
#include "Building_Nexus.generated.h"

UCLASS()
class LOL_UNREAL_API ABuilding_Nexus : public ABaseBuilding
{
	GENERATED_BODY()
public:
	ABuilding_Nexus();

private:
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, Category = "ABP")
	TSubclassOf<UAnimInstance> AnimBlueprint;
};
