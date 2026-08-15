// 롤 챔피언 투사체
#include "Champion/Projectile/BaseProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_StateComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "BaseChampion.h"
#include "Minion/BaseMinion.h"

#include "NiagaraComponent.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

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

    NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
    NiagaraComp->SetupAttachment(RootComponent);
    NiagaraComp->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.f; // 발사 초기 속도
	ProjectileMovement->MaxSpeed = 1000.f;     // 최대 속도
    ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void ABaseProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority() && bIsActive)
    {
        if (!IsValid(CurrentTarget))
        {
            Deactivate();
            return;
        }

        if (ULOL_StateComponent* TargetState = CurrentTarget->FindComponentByClass<ULOL_StateComponent>())
        {
            if (TargetState->HasStatusTag(LOLTags::State_Dead))
            {
                Deactivate();
                return;
            }
        }
    }
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
        if (Shooter)
        {
            ULOL_StateComponent* ShooterState = Shooter->FindComponentByClass<ULOL_StateComponent>();
            ULOL_StateComponent* TargetState = OtherActor->FindComponentByClass<ULOL_StateComponent>();

            if (ShooterState && TargetState && ShooterState->IsEnemy(TargetState))
            {
                if (ULOL_AttackComponent* AttackComp = Shooter->FindComponentByClass<ULOL_AttackComponent>())
                {
                    AttackComp->ExecuteAttackHit();
                }
                Deactivate();
            }
        }
    }
}

void ABaseProjectile::Deactivate()
{
    bIsActive = false;

    CurrentTarget = nullptr;
    SetActorTickEnabled(false);

    ApplyActiveState();
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->SetComponentTickEnabled(false);

    if (NiagaraComp)
    {
        NiagaraComp->Deactivate();
    }
}
void ABaseProjectile::Activate(FVector SpawnLocation, AActor* Target)
{
    bIsActive = true;

    CurrentTarget = Target;
    SetActorTickEnabled(true);

    SetActorLocation(SpawnLocation);
    ApplyActiveState();

    ProjectileMovement->SetUpdatedComponent(CollisionComp);

    if (Target)
    {
        FVector Direction = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
        ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
        FireAtTarget(Target);
    }
    else
    {
        ProjectileMovement->Velocity = FVector::ZeroVector;
        ProjectileMovement->bIsHomingProjectile = false;
    }

    ProjectileMovement->SetComponentTickEnabled(true);
    ProjectileMovement->Activate(true);

    if (NiagaraComp && NiagaraComp->GetAsset())
    {
        NiagaraComp->Activate(true);
        NiagaraComp->ReinitializeSystem();
    }
}

void ABaseProjectile::SetShooter(AActor* Actor)
{
    Shooter = Actor;
}

void ABaseProjectile::SetMesh(UStaticMesh* InMesh)
{
    ReplicatedMesh = InMesh;
    ReplicatedMeshScale = FVector::OneVector;
    ReplicatedMeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
    ApplyProjectileVisual();
}

void ABaseProjectile::SetMeshTransform(FVector InScale, FRotator InRelativeRotation)
{
    ReplicatedMeshScale = InScale;
    ReplicatedMeshRelativeRotation = InRelativeRotation;
    ApplyProjectileVisual();
}

void ABaseProjectile::ApplyProjectileVisual()
{
    if (NiagaraComp)
    {
        NiagaraComp->SetAsset(nullptr);
        NiagaraComp->SetVisibility(false);
    }

    if (MeshComp && ReplicatedMesh)
    {
        MeshComp->SetStaticMesh(ReplicatedMesh);
        MeshComp->SetRelativeScale3D(ReplicatedMeshScale);
        MeshComp->SetRelativeRotation(ReplicatedMeshRelativeRotation);
        MeshComp->SetVisibility(true);
    }
}

void ABaseProjectile::SetNiagara(UNiagaraSystem* InNiagara)
{
    ReplicatedMesh = nullptr;

    if (MeshComp)
    {
        MeshComp->SetStaticMesh(nullptr);
        MeshComp->SetVisibility(false);
    }

    if (NiagaraComp && InNiagara)
    {
        NiagaraComp->SetAsset(InNiagara);
        NiagaraComp->SetVisibility(true);
    }
}

void ABaseProjectile::OnRep_IsActive()
{
    ApplyActiveState();
}

void ABaseProjectile::OnRep_ProjectileVisual()
{
    ApplyProjectileVisual();
}

void ABaseProjectile::ApplyActiveState()
{
    SetActorHiddenInGame(!bIsActive);
    CollisionComp->SetCollisionEnabled(bIsActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void ABaseProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaseProjectile, bIsActive);
    DOREPLIFETIME(ABaseProjectile, ReplicatedMesh);
    DOREPLIFETIME(ABaseProjectile, ReplicatedMeshScale);
    DOREPLIFETIME(ABaseProjectile, ReplicatedMeshRelativeRotation);
}
