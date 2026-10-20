#include "Champion/Projectile/GragasRBarrelVisual.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AGragasRBarrelVisual::AGragasRBarrelVisual()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FlightRoot")));
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(RootComponent);
	BarrelMesh->SetMobility(EComponentMobility::Movable);
	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrelMesh->SetGenerateOverlapEvents(false);
}

void AGragasRBarrelVisual::Initialize(UStaticMesh* Mesh, UMaterialInterface* Material,
	const FVector& Start, const FVector& End, float Duration, float VisualRadius)
{
	StartLocation = Start;
	EndLocation = End;
	StartTime = GetWorld()->GetTimeSeconds();
	TravelDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	// Short throws stay low; long throws have a readable overhead arc.
	ArcHeight = FMath::Clamp(FVector::Dist2D(Start, End) * 0.32f, 35.0f, 300.0f);
	BarrelMesh->SetStaticMesh(Mesh);
	if (Material) BarrelMesh->SetMaterial(0, Material);
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const float Scale = VisualRadius / FMath::Max(static_cast<float>(Bounds.SphereRadius), 1.0f);
	MeshCenter = Bounds.Origin * Scale;
	BarrelMesh->SetRelativeScale3D(FVector(Scale));
	FVector Direction = End - Start;
	Direction.Z = 0.0f;
	SetActorRotation(Direction.Rotation());
	UpdateFlight(0.0f);
	SetLifeSpan(TravelDuration + 0.25f);
}

void AGragasRBarrelVisual::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Alpha = FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - StartTime) / TravelDuration, 0.0f, 1.0f);
	UpdateFlight(Alpha);
	// Finish on the same travel duration as the existing explosion timer.
	if (Alpha >= 1.0f) Destroy();
}

void AGragasRBarrelVisual::UpdateFlight(float Alpha)
{
	FVector Position = FMath::Lerp(StartLocation, EndLocation, Alpha);
	Position.Z += 4.0f * ArcHeight * Alpha * (1.0f - Alpha);
	SetActorLocation(Position);
	const FQuat Rotation = FQuat(FVector::RightVector, Alpha * 2.0f * PI) * FRotator(0, 90, 0).Quaternion();
	BarrelMesh->SetRelativeLocationAndRotation(-Rotation.RotateVector(MeshCenter), Rotation);
}
