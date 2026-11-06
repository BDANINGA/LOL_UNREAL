#include "Champion/AnimNotify_LeeSinTempest.h"
#include "Component/Champion_SkillComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UAnimNotify_LeeSinTempest::UAnimNotify_LeeSinTempest()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ground(
		TEXT("/Game/VFX/LeeSin/M_LeeSinTempestGround.M_LeeSinTempestGround"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Grit(
		TEXT("/Game/VFX/LeeSin/M_LeeSinTempestGrit.M_LeeSinTempestGrit"));
	GroundPlane = Plane.Object;
	GroundMaterial = Ground.Object;
	GritMaterial = Grit.Object;
}

void UAnimNotify_LeeSinTempest::Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(Mesh, Animation, EventReference);
	AActor* Caster = Mesh ? Mesh->GetOwner() : nullptr;
	if (!Caster || !GroundPlane || !GroundMaterial || !GritMaterial) return;
	UWorld* World = Caster->GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;

	const UCapsuleComponent* Capsule = Caster->FindComponentByClass<UCapsuleComponent>();
	FVector Ground = Capsule ? Capsule->GetComponentLocation() - FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight()
	                         : Mesh->GetComponentLocation();
	FVector Normal = FVector::UpVector;
	FHitResult Floor;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(LeeSinTempestFloor), false, Caster);
	if (World->LineTraceSingleByObjectType(Floor, Ground + FVector::UpVector * 80.f, Ground - FVector::UpVector * 180.f,
		FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		Ground = Floor.ImpactPoint;
		Normal = Floor.ImpactNormal;
	}

	float Radius = 350.f;
	if (auto* Skills = Caster->FindComponentByClass<UChampion_SkillComponent>())
	{
		const auto& Range = Skills->GetE_Data().Range;
		if (!Range.IsEmpty()) Radius = Range[0];
	}
	const float StartTime = World->GetTimeSeconds();
	auto* Wave = NewObject<UStaticMeshComponent>(Caster);
	Wave->SetStaticMesh(GroundPlane);
	auto* WaveMID = Wave->CreateDynamicMaterialInstance(0, GroundMaterial);
	WaveMID->SetScalarParameterValue(TEXT("StartTime"), StartTime);
	Wave->SetWorldLocationAndRotation(Ground + Normal * 4.f,
		FRotationMatrix::MakeFromZX(Normal, Caster->GetActorForwardVector()).Rotator());
	// The leading crest finishes at 93% of the plane's half-width.
	const float Scale = FMath::Max(1.f, Radius) / 46.5f;
	Wave->SetWorldScale3D(FVector(Scale, Scale, 1.f));

	auto* Grit = NewObject<UMaterialBillboardComponent>(Caster);
	auto* GritMID = UMaterialInstanceDynamic::Create(GritMaterial, Grit);
	GritMID->SetScalarParameterValue(TEXT("StartTime"), StartTime);
	Grit->AddElement(GritMID, nullptr, false, 160.f, 96.f, nullptr);
	Grit->SetWorldLocation(Ground + Normal * 65.f);

	for (UPrimitiveComponent* Layer : {static_cast<UPrimitiveComponent*>(Wave), static_cast<UPrimitiveComponent*>(Grit)})
	{
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetGenerateOverlapEvents(false);
		Layer->SetCanEverAffectNavigation(false);
		Layer->SetCastShadow(false);
		Layer->bReceivesDecals = false;
		Layer->RegisterComponent();
		FTimerHandle Lifetime;
		World->GetTimerManager().SetTimer(Lifetime,
			FTimerDelegate::CreateWeakLambda(Layer, [Layer]() { Layer->DestroyComponent(); }), .55f, false);
	}
}
