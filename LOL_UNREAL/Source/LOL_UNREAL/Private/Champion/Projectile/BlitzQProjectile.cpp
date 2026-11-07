#include "Champion/Projectile/BlitzQProjectile.h"

#include "Champion/Champion_Blitz.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ABlitzQProjectile::ABlitzQProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(60.0f);

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HookRoot"));
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookMesh"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
}

void ABlitzQProjectile::Initialize(AChampion_Blitz* InShooter, float InSpeed, float InRange,
    float InRadius, UStaticMesh* InMesh, float InVisualRadius, float InVisualScale, FRotator InRotationOffset)
{
    Shooter = InShooter;
    Direction = GetActorForwardVector().GetSafeNormal2D();
    Speed = FMath::Max(InSpeed, 1.0f);
    RemainingDistance = FMath::Max(InRange, 0.0f);
    Radius = FMath::Max(InRadius, 1.0f);
    VisualMesh = InMesh;
    VisualRotation = InRotationOffset;
    const float MeshRadius = InMesh ? InMesh->GetBounds().SphereRadius : 0.0f;
    VisualScale = (MeshRadius > KINDA_SMALL_NUMBER ? InVisualRadius / MeshRadius : 1.0f) * InVisualScale;
    OnRep_Visual();
}

void ABlitzQProjectile::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(HasAuthority());
}

void ABlitzQProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!HasAuthority() || bFinished) return;

    AChampion_Blitz* Blitz = Shooter.Get();
    if (!IsValid(Blitz) || RemainingDistance <= 0.0f)
    {
        bFinished = true;
        Destroy();
        return;
    }

    const FVector Start = GetActorLocation();
    const float StepDistance = FMath::Min(Speed * FMath::Max(DeltaTime, 0.0f), RemainingDistance);
    const FVector End = Start + Direction * StepDistance;

    // Champions use a custom object channel; minions and monsters use Pawn.
    // An object query returns all candidates, so allies cannot hide an enemy behind them.
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    Objects.AddObjectTypesToQuery(ECC_GameTraceChannel2);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BlitzQProjectile), false, this);
    Query.AddIgnoredActor(Blitz);
    TArray<FHitResult> Hits;
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(Radius), Query);
    Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Time < B.Time; });

    for (const FHitResult& Hit : Hits)
    {
        ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
        // Ignore attack-range sensors and meshes; the target's body must actually be hit.
        if (!Target || Hit.GetComponent() != Target->GetCapsuleComponent() ||
            !Blitz->IsValidBlitzSkillTarget(Target)) continue;

        bFinished = true;
        SetActorLocation(Hit.Location);
        Blitz->BeginPullTarget(Target);
        Destroy();
        return;
    }

    SetActorLocation(End);
    RemainingDistance -= StepDistance;
    if (RemainingDistance <= KINDA_SMALL_NUMBER)
    {
        bFinished = true;
        Destroy();
    }
}

void ABlitzQProjectile::OnRep_Visual()
{
    MeshComponent->SetStaticMesh(VisualMesh);
    MeshComponent->SetRelativeScale3D(FVector(VisualScale));
    MeshComponent->SetRelativeRotation(VisualRotation);
}

void ABlitzQProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABlitzQProjectile, VisualMesh);
    DOREPLIFETIME(ABlitzQProjectile, VisualScale);
    DOREPLIFETIME(ABlitzQProjectile, VisualRotation);
}
