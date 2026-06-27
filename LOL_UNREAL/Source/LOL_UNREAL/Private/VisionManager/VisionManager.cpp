#include "VisionManager/VisionManager.h"

#include "Building/BaseBuilding.h"

#include "Component/LOL_VisionComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AVisionManager::AVisionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
}

void AVisionManager::BeginPlay()
{
	Super::BeginPlay();
	if (VisionBrushMaterial)
	{
		VisionBrushMID = UMaterialInstanceDynamic::Create(VisionBrushMaterial, this);
	}
}

void AVisionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFoW();
}

void AVisionManager::RegisterVisionComponent(ULOL_VisionComponent* Component)
{
	if (!Component)
		return;

	ULOL_StateComponent* State =
		Component->GetOwner()->FindComponentByClass<ULOL_StateComponent>();

	if (!State)
		return;

	if (State->HasStatusTag(LOLTags::Team_Blue))
	{
		BlueVisionComponents.AddUnique(Component);
	}
	else if (State->HasStatusTag(LOLTags::Team_Red))
	{
		RedVisionComponents.AddUnique(Component);
	}
}

void AVisionManager::UnregisterVisionComponent(ULOL_VisionComponent* Component)
{
	BlueVisionComponents.Remove(Component);
	RedVisionComponents.Remove(Component);
}

void AVisionManager::UpdateFoW()
{
	if (!FoWRenderTarget || !VisionBrushMID) return;

	UKismetRenderingLibrary::ClearRenderTarget2D(this, FoWRenderTarget, FLinearColor::Black);
	FVector2D MapSize = MapMaxBounds - MapMinBounds;

	APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!LocalPC) return;

	AActor* LocalPlayerPawn = LocalPC->GetPawn();
	if (!LocalPlayerPawn) return;

	ULOL_StateComponent* LocalPlayerState =
		LocalPlayerPawn->FindComponentByClass<ULOL_StateComponent>();
	if (!LocalPlayerState) return;

	const TArray<ULOL_VisionComponent*>* ActiveVisionComponents = nullptr;

	if (LocalPlayerState->HasStatusTag(LOLTags::Team_Blue))
	{
		ActiveVisionComponents = &BlueVisionComponents;
	}
	else if (LocalPlayerState->HasStatusTag(LOLTags::Team_Red))
	{
		ActiveVisionComponents = &RedVisionComponents;
	}

	for (ULOL_VisionComponent* VisionComp : *ActiveVisionComponents)
	{
		if (!IsValid(VisionComp)) continue;

		AActor* OwnerActor = VisionComp->GetOwner();
		if (!OwnerActor) continue;

		ULOL_StateComponent* OwnerState = OwnerActor->FindComponentByClass<ULOL_StateComponent>();




		FVector WorldLoc = VisionComp->GetOwner()->GetActorLocation();

		float U = (WorldLoc.X - MapMinBounds.X) / MapSize.X;
		float V = (WorldLoc.Y - MapMinBounds.Y) / MapSize.Y;

		VisionBrushMID->SetVectorParameterValue(FName("DrawPosition"), FLinearColor(U, V, 0.0f, 1.0f));

		float RadiusUV = VisionComp->VisionRadius / FMath::Max(MapSize.X, MapSize.Y);
		VisionBrushMID->SetScalarParameterValue(FName("VisionRadius"), RadiusUV);

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, FoWRenderTarget, VisionBrushMID);
	}
}