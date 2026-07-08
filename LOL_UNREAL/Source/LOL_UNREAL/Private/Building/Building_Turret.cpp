#include "Building/Building_Turret.h"
#include "Building/LOL_TurretAIController.h"

ABuilding_Turret::ABuilding_Turret()
{
    PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

    AIControllerClass = ALOL_TurretAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    BuildingName = TEXT("Building_Turret");
}
