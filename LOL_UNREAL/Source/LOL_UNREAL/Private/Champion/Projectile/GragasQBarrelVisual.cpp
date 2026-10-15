#include "Champion/Projectile/GragasQBarrelVisual.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

AGragasQBarrelVisual::AGragasQBarrelVisual()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RollRoot")));
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(RootComponent);
	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrelMesh->SetGenerateOverlapEvents(false);
}

void AGragasQBarrelVisual::Initialize(UStaticMesh* Mesh, UMaterialInterface* Material,
	const FVector& Start, const FVector& End, float Duration, float VisualRadius, float LifeTime)
{
	StartLocation = Start;
	EndLocation = End;
	TravelDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	BarrelMesh->SetStaticMesh(Mesh);
	if (Material) BarrelMesh->SetMaterial(0, Material);
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	MeshScale = VisualRadius / FMath::Max(static_cast<float>(Bounds.SphereRadius), 1.0f);
	MeshCenter = Bounds.Origin * MeshScale;
	// throw_q's cylinder axis is local X; its Y/Z extents form the rolling surface.
	RollingRadius = FMath::Max(static_cast<float>(FMath::Max(Bounds.BoxExtent.Y, Bounds.BoxExtent.Z)) * MeshScale, 1.0f);
	BarrelMesh->SetRelativeScale3D(FVector(MeshScale));
	FVector Direction = End - Start;
	Direction.Z = 0.0f;
	SetActorRotation(Direction.Rotation());
	FallbackGroundZ = Start.Z - RollingRadius;
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		FallbackGroundZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	UpdateVisual(0.0f);
	SetLifeSpan(FMath::Max(LifeTime, 0.2f));
}

void AGragasQBarrelVisual::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / TravelDuration, 0.0f, 1.0f);
	UpdateVisual(Alpha);
	if (Alpha >= 1.0f) SetActorTickEnabled(false);
}

void AGragasQBarrelVisual::UpdateVisual(float Alpha)
{
	FVector Position = FMath::Lerp(StartLocation, EndLocation, Alpha);
	FHitResult GroundHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GragasQGround), false, this);
	Params.AddIgnoredActor(GetOwner());
	const bool bGround = GetWorld()->LineTraceSingleByObjectType(GroundHit,
		Position + FVector(0, 0, 300), Position - FVector(0, 0, 1500),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	Position.Z = (bGround ? GroundHit.ImpactPoint.Z : FallbackGroundZ) + RollingRadius + 2.0f;
	SetActorLocation(Position);
	const float Angle = FVector::Dist2D(StartLocation, EndLocation) * Alpha / RollingRadius;
	const FQuat Rotation = FQuat(FVector::RightVector, Angle) * FRotator(0, 90, 0).Quaternion();
	// Rotate about the barrel's center even when the imported mesh pivot is offset.
	BarrelMesh->SetRelativeLocationAndRotation(-Rotation.RotateVector(MeshCenter), Rotation);
}
