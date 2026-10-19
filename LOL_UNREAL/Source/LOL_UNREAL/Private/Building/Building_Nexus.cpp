#include "Building/Building_Nexus.h"
#include "LOL_GameModeBase.h"
#include "Engine/World.h"

ABuilding_Nexus::ABuilding_Nexus()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

	BuildingName = TEXT("Building_Nexus");
}

void ABuilding_Nexus::OnBuildingDeath()
{
	Super::OnBuildingDeath();
	if (HasAuthority())
	{
		if (ALOL_GameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALOL_GameModeBase>())
		{
			GameMode->NotifyNexusDestroyed(this);
		}
	}
}
