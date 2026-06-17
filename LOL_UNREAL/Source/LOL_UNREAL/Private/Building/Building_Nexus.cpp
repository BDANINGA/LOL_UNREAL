#include "Building/Building_Nexus.h"

ABuilding_Nexus::ABuilding_Nexus()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

	BuildingName = TEXT("Building_Nexus");
}
