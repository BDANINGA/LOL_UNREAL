#include "Champion/Projectile/BlitzQProjectile.h"

#include "Champion/Champion_Blitz.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
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

    CableComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("GrabCable"));
    CableComponent->SetMobility(EComponentMobility::Movable);
    CableComponent->SetupAttachment(RootComponent);
    CableComponent->SetCanEverAffectNavigation(false);
    CableComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CableComponent->SetGenerateOverlapEvents(false);
    CableComponent->SetCastShadow(false);
    CableComponent->SetVisibility(false);
    CableComponent->SetForwardAxis(ESplineMeshAxis::Z, false);
    CableComponent->SetStartScale(FVector2D(0.075f), false);
    CableComponent->SetEndScale(FVector2D(0.075f), false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CableMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> CableMaterial(TEXT("/Game/VFX/Blitz/M_BlitzGrabCable.M_BlitzGrabCable"));
    if (CableMesh.Succeeded()) CableComponent->SetStaticMesh(CableMesh.Object);
    if (CableMaterial.Succeeded()) CableComponent->SetMaterial(0, CableMaterial.Object);
}

void ABlitzQProjectile::Initialize(AChampion_Blitz* InShooter, float InSpeed, float InRange,
    float InRadius, UStaticMesh* InMesh, float InVisualRadius, float InVisualScale, FRotator InRotationOffset)
{
    SetOwner(InShooter);
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
    // Allow a return flight even if a target dies during the pull.
    if (HasAuthority()) SetLifeSpan(RemainingDistance / Speed + 3.0f);
}

void ABlitzQProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AChampion_Blitz* Blitz = Cast<AChampion_Blitz>(GetOwner());
    if (!HasAuthority())
    {
        UpdateCable(Blitz);
        return;
    }

    if (!IsValid(Blitz))
    {
        Destroy();
        return;
    }

    if (bReturning)
    {
        if (ReturnTarget.IsValid() && Blitz->GrabbedTarget == ReturnTarget.Get())
        {
            SetActorLocation(ReturnTarget->GetActorLocation() + GrabOffset);
        }
        else
        {
            const FVector Origin = GetCableOrigin(Blitz);
            const FVector Position = FMath::VInterpConstantTo(GetActorLocation(), Origin,
                DeltaTime, Speed * 1.7f);
            SetActorLocation(Position);
            if (Position.Equals(Origin, 10.0f))
            {
                Destroy();
                return;
            }
        }
        UpdateCable(Blitz);
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

        SetActorLocation(Hit.Location);
        Blitz->BeginPullTarget(Target);
        BeginReturn(Blitz->GrabbedTarget == Target ? Target : nullptr);
        UpdateCable(Blitz);
        return;
    }

    SetActorLocation(End);
    RemainingDistance -= StepDistance;
    if (RemainingDistance <= KINDA_SMALL_NUMBER)
    {
        BeginReturn();
    }
    UpdateCable(Blitz);
}

void ABlitzQProjectile::BeginReturn(ACharacter* Target)
{
    bReturning = true;
    ReturnTarget = Target;
    if (Target) GrabOffset = GetActorLocation() - Target->GetActorLocation();
    ForceNetUpdate();
}

FVector ABlitzQProjectile::GetCableOrigin(const AChampion_Blitz* Blitz) const
{
    if (Blitz->GetMesh() && Blitz->GetMesh()->DoesSocketExist(TEXT("R_hand")))
    {
        return Blitz->GetMesh()->GetSocketLocation(TEXT("R_hand"));
    }
    return Blitz->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
}

void ABlitzQProjectile::UpdateCable(const AChampion_Blitz* Blitz)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    CableComponent->SetVisibility(IsValid(Blitz));
    if (!IsValid(Blitz)) return;

    const FTransform Transform = GetActorTransform();
    const FVector Start = Transform.InverseTransformPosition(GetCableOrigin(Blitz));
    // The cable plugs into the rear cuff of the fist.
    const FVector End = MeshComponent->GetRelativeTransform().TransformPosition(FVector(-30.0f, 0.0f, 0.0f));
    const FVector Tangent = End - Start;
    const float Sag = FMath::Min(Tangent.Size() * 0.12f, 45.0f);
    const FVector Down = Transform.InverseTransformVectorNoScale(FVector(0.0f, 0.0f, -Sag));
    CableComponent->SetStartAndEnd(Start, Tangent + Down, End, Tangent - Down);
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
