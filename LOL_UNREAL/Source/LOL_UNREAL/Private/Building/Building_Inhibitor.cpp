#include "Building/Building_Inhibitor.h"

ABuilding_Inhibitor::ABuilding_Inhibitor()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

	BuildingName = TEXT("Building_Inhibitor");
}
