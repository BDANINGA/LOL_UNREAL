#include "Building/Building_Turret.h"
#include "Building/LOL_TurretAIController.h"
#include "Components/SkeletalMeshComponent.h"

ABuilding_Turret::ABuilding_Turret()
{
    PrimaryActorTick.bCanEverTick = false;

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RootComponent);

    BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BuildingMesh->SetCollisionResponseToChannel(
        ECC_Visibility,
        ECR_Block
    );

    DestructionMesh = CreateDefaultSubobject<USkeletalMeshComponent>(
        TEXT("DestructionMesh")
    );

    DestructionMesh->SetupAttachment(RootComponent);
        
    DestructionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DestructionMesh->SetSimulatePhysics(false);

    AIControllerClass = ALOL_TurretAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    BuildingName = TEXT("Building_Turret");

    
}

void ABuilding_Turret::OnBuildingDeath()
{
    if (!BuildingMesh || !DestructionMesh)
    {
        return;
    }

    // 기존 포탑 숨김
    BuildingMesh->SetVisibility(false);

    // 파괴용 스켈레탈 메쉬 표시
    DestructionMesh->SetVisibility(true);
    
    DestructionMesh->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics
    );

    // 피직스 시뮬레이션 시작
    DestructionMesh->SetSimulatePhysics(true);

    DestructionMesh->WakeAllRigidBodies();
}

USkeletalMeshComponent* ABuilding_Turret::GetDestructionMesh() const
{
    return DestructionMesh;
}
