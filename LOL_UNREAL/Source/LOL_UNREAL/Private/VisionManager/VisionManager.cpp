// Fill out your copyright notice in the Description page of Project Settings.


#include "VisionManager/VisionManager.h"

#include "BaseChampion.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

AVisionManager::AVisionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AVisionManager::BeginPlay()
{
	Super::BeginPlay();
}

void AVisionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetNetMode() == NM_DedicatedServer) return;

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate < UpdateInterval) return;

	TimeSinceLastUpdate = 0.0f;
	UpdateVisionForLocalPlayer();
}

void AVisionManager::UpdateVisionForLocalPlayer()
{
	ABaseChampion* LocalChampion = GetLocalChampion();
	if (!IsValid(LocalChampion)) return;

	TArray<ABaseChampion*> VisionSources;
	for (TActorIterator<ABaseChampion> It(GetWorld()); It; ++It)
	{
		ABaseChampion* Champion = *It;
		if (!IsValid(Champion)) continue;

		if (Champion->TeamId == LocalChampion->TeamId)
		{
			VisionSources.Add(Champion);
			Champion->SetVisibleByVision(true);

			if (bDrawDebugVision)
			{
				DrawDebugSphere(
					GetWorld(),
					Champion->GetActorLocation(),
					Champion->SightRadius,
					32,
					FColor::Green,
					false,
					UpdateInterval
				);
			}
		}
	}

	if (!bHideEnemyChampions) return;

	for (TActorIterator<ABaseChampion> It(GetWorld()); It; ++It)
	{
		ABaseChampion* Champion = *It;
		if (!IsValid(Champion) || Champion == LocalChampion) continue;
		if (Champion->TeamId == LocalChampion->TeamId) continue;

		const bool bVisible = IsLocationVisible(
			Champion->GetActorLocation(),
			VisionSources
		);
		Champion->SetVisibleByVision(bVisible);
	}
}

bool AVisionManager::IsLocationVisible(
	const FVector& Location,
	const TArray<ABaseChampion*>& VisionSources) const
{
	for (ABaseChampion* Source : VisionSources)
	{
		if (!IsValid(Source)) continue;

		const float Radius = FMath::Max(Source->SightRadius, 0.0f);
		if (FVector::DistSquared2D(Source->GetActorLocation(), Location) <=
			FMath::Square(Radius))
		{
			return true;
		}
	}

	return false;
}

ABaseChampion* AVisionManager::GetLocalChampion() const
{
	if (!GetWorld()) return nullptr;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController()) continue;

		return Cast<ABaseChampion>(PlayerController->GetPawn());
	}

	return nullptr;
}

