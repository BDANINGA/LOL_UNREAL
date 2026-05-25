// 롤 챔피언 투사체
#include "Champion/Projectile/BaseProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BaseChampion.h"
#include "Component/LOL_AttackComponent.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(15.0f);
	CollisionComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.f; // 발사 초기 속도
	ProjectileMovement->MaxSpeed = 1000.f;     // 최대 속도
    ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnOverlapBegin);
}

void ABaseProjectile::FireAtTarget(AActor* TargetActor)
{
    if (!TargetActor || !ProjectileMovement) return;

    ProjectileMovement->bIsHomingProjectile = true;
    ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
    ProjectileMovement->HomingAccelerationMagnitude = 10000.f; 
}

void ABaseProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this && OtherActor != Shooter)
    {
        if (ABaseChampion* HitChampion = Cast<ABaseChampion>(OtherActor))
        {
            // 적중 이펙트나 사운드 Multicast
            if (ABaseChampion* OwnerChampion = Cast<ABaseChampion>(Shooter))
                OwnerChampion->AttackComponent->ExecuteAttackHit();

            Deactivate();
        }
    }
}

void ABaseProjectile::Deactivate()
{
    bIsActive = false;

    SetActorHiddenInGame(true);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->SetComponentTickEnabled(false);
}
void ABaseProjectile::Activate(FVector SpawnLocation, AActor* Target)
{
    bIsActive = true;

    SetActorLocation(SpawnLocation);
    SetActorHiddenInGame(false);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectileMovement->Velocity = FVector::ZeroVector;
    ProjectileMovement->SetComponentTickEnabled(true);
    ProjectileMovement->Activate();
    FireAtTarget(Target);
}

void ABaseProjectile::SetShooter(AActor* Actor)
{
    Shooter = Actor;
}

void ABaseProjectile::SetMesh(UStaticMesh* InMesh)
{
    MeshComp->SetStaticMesh(InMesh);
}
