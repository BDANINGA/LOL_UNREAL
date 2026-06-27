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
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.f,
				FColor::Green,
				Component->GetOwner()->GetName());
		}
		return;
	}
		

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

void AVisionManager::RegisterActor(AActor* Actor)
{
	if (!IsValid(Actor))
		return;

	ULOL_StateComponent* State =
		Actor->FindComponentByClass<ULOL_StateComponent>();

	if (!State)
		return;

	if (State->HasStatusTag(LOLTags::Team_Blue))
	{
		BlueActors.AddUnique(Actor);
	}
	else if (State->HasStatusTag(LOLTags::Team_Red))
	{
		RedActors.AddUnique(Actor);
	}
}
void AVisionManager::UnregisterActor(AActor* Actor)
{
	BlueActors.Remove(Actor);
	RedActors.Remove(Actor);
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
	const TArray<AActor*>* EnemyActors = nullptr;

	if (LocalPlayerState->HasStatusTag(LOLTags::Team_Blue))
	{
		ActiveVisionComponents = &BlueVisionComponents;
		EnemyActors = &RedActors;
	}
	else if (LocalPlayerState->HasStatusTag(LOLTags::Team_Red))
	{
		ActiveVisionComponents = &RedVisionComponents;
		EnemyActors = &BlueActors;
	}

	for (ULOL_VisionComponent* VisionComp : *ActiveVisionComponents)
	{
		if (!IsValid(VisionComp)) continue;

		FVector WorldLoc = VisionComp->GetOwner()->GetActorLocation();

		float U = (WorldLoc.X - MapMinBounds.X) / MapSize.X;
		float V = (WorldLoc.Y - MapMinBounds.Y) / MapSize.Y;

		VisionBrushMID->SetVectorParameterValue(FName("DrawPosition"), FLinearColor(U, V, 0.0f, 1.0f));

		float RadiusUV = VisionComp->VisionRadius / FMath::Max(MapSize.X, MapSize.Y);
		VisionBrushMID->SetScalarParameterValue(FName("VisionRadius"), RadiusUV);

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, FoWRenderTarget, VisionBrushMID);
	}

	for (AActor* Enemy : *EnemyActors)
	{
		if (!IsValid(Enemy))
			continue;

		bool bVisible = false;

		for (ULOL_VisionComponent* VisionComp : *ActiveVisionComponents)
		{
			if (!IsValid(VisionComp))
				continue;

			float Dist = FVector::Dist(
				VisionComp->GetOwner()->GetActorLocation(),
				Enemy->GetActorLocation());

			if (Dist <= VisionComp->VisionRadius)
			{
				bVisible = true;
				break;
			}
		}

		Enemy->SetActorHiddenInGame(!bVisible);
	}
}