#include "Building/Building_Turret.h"
#include "Building/LOL_TurretAIController.h"
#include "DrawDebugHelpers.h"

ABuilding_Turret::ABuilding_Turret()
{
    PrimaryActorTick.bCanEverTick = true;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

    AIControllerClass = ALOL_TurretAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    BuildingName = TEXT("Building_Turret");
}
void ABuilding_Turret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsValid(CurrentDebugTarget))
	{
		FVector StartLoc = GetActorLocation() + FVector(0, 0, 300); 

		if (UStaticMeshComponent* MeshComp = FindComponentByClass<UStaticMeshComponent>())
		{
			if (MeshComp->DoesSocketExist(TEXT("FirePoint")))
			{
				StartLoc = MeshComp->GetSocketLocation(TEXT("FirePoint"));
			}
		}

		FVector EndLoc = CurrentDebugTarget->GetActorLocation();

		DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Red, false, 0.0f, 0, 5.0f);
	}
}