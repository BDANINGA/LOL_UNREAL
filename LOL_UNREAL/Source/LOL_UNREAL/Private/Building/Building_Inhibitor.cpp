#include "Building/Building_Inhibitor.h"
#include "LOL_GameModeBase.h"
#include "Engine/World.h"

ABuilding_Inhibitor::ABuilding_Inhibitor()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

	BuildingName = TEXT("Building_Inhibitor");
}

void ABuilding_Inhibitor::OnBuildingDeath()
{
	Super::OnBuildingDeath();
	if (HasAuthority())
	{
		if (ALOL_GameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALOL_GameModeBase>())
		{
			GameMode->NotifyInhibitorDestroyed(this);
		}
	}
}
