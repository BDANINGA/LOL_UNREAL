#include "VisionManager/VisionManager.h"

#include "BaseChampion.h"
#include "Component/LOL_StateComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Minion/BaseMinion.h"
#include "UObject/UObjectIterator.h"

//AVisionManager::AVisionManager()
//{
//	PrimaryActorTick.bCanEverTick = true;
//	PrimaryActorTick.bStartWithTickEnabled = true;
//	bReplicates = false;
//
//	VisionTextureParameterNames = {
//		TEXT("FOWTexture"),
//		TEXT("FOWMask"),
//		TEXT("VisionMask"),
//		TEXT("FogOfWarTexture")
//	};
//}
//
//void AVisionManager::BeginPlay()
//{
//	Super::BeginPlay();
//
//	if (GetNetMode() == NM_DedicatedServer)
//	{
//		SetActorTickEnabled(false);
//		return;
//	}
//
//	CanvasWhiteTexture = LoadObject<UTexture2D>(
//		nullptr,
//		TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")
//	);
//
//	VisionRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
//		this,
//		UCanvasRenderTarget2D::StaticClass(),
//		RenderTargetSize,
//		RenderTargetSize
//	);
//
//	if (VisionRenderTarget)
//	{
//		VisionRenderTarget->ClearColor = FLinearColor(
//			HiddenFowValue,
//			HiddenFowValue,
//			HiddenFowValue,
//			1.0f
//		);
//		VisionRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(
//			this,
//			&AVisionManager::DrawVisionMask
//		);
//		VisionRenderTarget->UpdateResource();
//	}
//
//	if (bApplyRenderTargetToWorldMaterials)
//	{
//		ApplyRenderTargetToWorldMaterials();
//	}
//}
//
//void AVisionManager::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	if (!VisionRenderTarget)
//	{
//		return;
//	}
//
//	RefreshVision();
//}
//
//void AVisionManager::RefreshVision()
//{
//	LocalChampion = ResolveLocalChampion();
//	CollectVisionSources();
//
//	VisionRenderTarget->UpdateResource();
//
//	if (bHideEnemiesOutsideVision)
//	{
//		UpdateActorVisibility();
//	}
//}
//
//ABaseChampion* AVisionManager::ResolveLocalChampion() const
//{
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return nullptr;
//	}
//
//	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
//	if (!PlayerController)
//	{
//		PlayerController = World->GetFirstPlayerController();
//	}
//	return PlayerController
//		? Cast<ABaseChampion>(PlayerController->GetPawn())
//		: nullptr;
//}
//
//ULOL_StateComponent* AVisionManager::GetLocalState() const
//{
//	return LocalChampion ? LocalChampion->StateComponent : nullptr;
//}
//
//void AVisionManager::CollectVisionSources()
//{
//	CachedVisionSources.Reset();
//
//	UWorld* World = GetWorld();
//	ULOL_StateComponent* LocalState = GetLocalState();
//	if (!World || !LocalState)
//	{
//		return;
//	}
//
//	if (bUseChampionVision)
//	{
//		TArray<AActor*> Champions;
//		UGameplayStatics::GetAllActorsOfClass(World, ABaseChampion::StaticClass(), Champions);
//		for (AActor* Actor : Champions)
//		{
//			ABaseChampion* Champion = Cast<ABaseChampion>(Actor);
//			if (!Champion || !IsFriendlyVisionSource(Champion, LocalState))
//			{
//				continue;
//			}
//
//			CachedVisionSources.Add({ Champion->GetActorLocation(), Champion->SightRadius });
//		}
//	}
//
//	if (bUseMinionVision)
//	{
//		TArray<AActor*> Minions;
//		UGameplayStatics::GetAllActorsOfClass(World, ABaseMinion::StaticClass(), Minions);
//		for (AActor* Actor : Minions)
//		{
//			ABaseMinion* Minion = Cast<ABaseMinion>(Actor);
//			if (!Minion || !IsFriendlyVisionSource(Minion, LocalState))
//			{
//				continue;
//			}
//
//			CachedVisionSources.Add({ Minion->GetActorLocation(), Minion->SightRadius });
//		}
//	}
//}
//
//bool AVisionManager::IsFriendlyVisionSource(AActor* Actor, ULOL_StateComponent* LocalState) const
//{
//	if (!Actor || !LocalState)
//	{
//		return false;
//	}
//
//	if (Actor == LocalChampion)
//	{
//		return true;
//	}
//
//	ULOL_StateComponent* ActorState = Actor->FindComponentByClass<ULOL_StateComponent>();
//	if (!ActorState || ActorState->HasStatusTag(LOLTags::Team_Jungle))
//	{
//		return false;
//	}
//
//	return !LocalState->IsEnemy(ActorState);
//}
//
//bool AVisionManager::ShouldControlVisibility(AActor* Actor, ULOL_StateComponent* LocalState) const
//{
//	if (!Actor || !LocalState || Actor == LocalChampion)
//	{
//		return false;
//	}
//
//	ULOL_StateComponent* ActorState = Actor->FindComponentByClass<ULOL_StateComponent>();
//	if (!ActorState)
//	{
//		return false;
//	}
//
//	return LocalState->IsEnemy(ActorState);
//}
//
//void AVisionManager::UpdateActorVisibility()
//{
//	// Listen-server host shares the authoritative world with clients.
//	// Hiding actors there can make another client lose their own pawn.
//	if (GetLocalRole() == ROLE_Authority)
//	{
//		return;
//	}
//
//	UWorld* World = GetWorld();
//	ULOL_StateComponent* LocalState = GetLocalState();
//	if (!World || !LocalState)
//	{
//		return;
//	}
//
//	TArray<AActor*> ControlledActors;
//	UGameplayStatics::GetAllActorsOfClass(World, ABaseChampion::StaticClass(), ControlledActors);
//
//	TArray<AActor*> Minions;
//	UGameplayStatics::GetAllActorsOfClass(World, ABaseMinion::StaticClass(), Minions);
//	ControlledActors.Append(Minions);
//
//	TArray<AActor*> JungleMonsters;
//	UGameplayStatics::GetAllActorsOfClass(World, ABaseJungleMonster::StaticClass(), JungleMonsters);
//	ControlledActors.Append(JungleMonsters);
//
//	for (AActor* Actor : ControlledActors)
//	{
//		if (!ShouldControlVisibility(Actor, LocalState))
//		{
//			continue;
//		}
//
//		const bool bVisible = IsWorldLocationVisible(Actor->GetActorLocation());
//		Actor->SetActorHiddenInGame(!bVisible);
//		Actor->SetActorEnableCollision(bVisible);
//
//		if (ABaseChampion* Champion = Cast<ABaseChampion>(Actor))
//		{
//			Champion->bVisibleByVision = bVisible;
//		}
//	}
//}
//
//bool AVisionManager::IsWorldLocationVisible(const FVector& WorldLocation) const
//{
//	for (const FVisionSource& Source : CachedVisionSources)
//	{
//		if (FVector::Dist2D(Source.Location, WorldLocation) > Source.Radius)
//		{
//			continue;
//		}
//
//		if (HasLineOfSight(Source.Location, WorldLocation))
//		{
//			return true;
//		}
//	}
//
//	return false;
//}
//
//bool AVisionManager::HasLineOfSight(const FVector& From, const FVector& To) const
//{
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return false;
//	}
//
//	FVector Start = From;
//	FVector End = To;
//	Start.Z += VisionTraceHeight;
//	End.Z += VisionTraceHeight;
//
//	FCollisionQueryParams Params(SCENE_QUERY_STAT(VisionLineOfSight), false);
//	if (LocalChampion)
//	{
//		Params.AddIgnoredActor(LocalChampion);
//	}
//
//	FHitResult Hit;
//	FCollisionObjectQueryParams ObjectParams;
//	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
//
//	return !World->LineTraceSingleByObjectType(
//		Hit,
//		Start,
//		End,
//		ObjectParams,
//		Params
//	);
//}
//
//void AVisionManager::DrawVisionMask(UCanvas* Canvas, int32 Width, int32 Height)
//{
//	if (!Canvas)
//	{
//		return;
//	}
//
//	TArray<FCanvasUVTri> Triangles;
//	Triangles.Reserve(6 + CachedVisionSources.Num() * VisionTraceSegments);
//
//	const FLinearColor HiddenColor(
//		HiddenFowValue,
//		HiddenFowValue,
//		HiddenFowValue,
//		1.0f
//	);
//
//	const FVector2D TopLeft(0.0f, 0.0f);
//	const FVector2D TopRight(Width, 0.0f);
//	const FVector2D BottomLeft(0.0f, Height);
//	const FVector2D BottomRight(Width, Height);
//	AddTriangle(Triangles, TopLeft, TopRight, BottomRight, HiddenColor);
//	AddTriangle(Triangles, TopLeft, BottomRight, BottomLeft, HiddenColor);
//
//	for (const FVisionSource& Source : CachedVisionSources)
//	{
//		AddVisionSourceTriangles(Triangles, Source, Width, Height);
//	}
//
//	Canvas->K2_DrawTriangle(CanvasWhiteTexture.Get(), Triangles);
//}
//
//void AVisionManager::AddVisionSourceTriangles(
//	TArray<FCanvasUVTri>& Triangles,
//	const FVisionSource& Source,
//	int32 Width,
//	int32 Height) const
//{
//	if (Source.Radius <= 0.0f || VisionTraceSegments < 3)
//	{
//		return;
//	}
//
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return;
//	}
//
//	const FVector2D CanvasCenter = WorldToCanvas(Source.Location, Width, Height);
//	TArray<FVector2D> Points;
//	Points.Reserve(VisionTraceSegments);
//
//	FCollisionQueryParams Params(SCENE_QUERY_STAT(VisionMaskTrace), false);
//	if (LocalChampion)
//	{
//		Params.AddIgnoredActor(LocalChampion);
//	}
//
//	FCollisionObjectQueryParams ObjectParams;
//	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
//
//	const FVector TraceStart = Source.Location + FVector(0.0f, 0.0f, VisionTraceHeight);
//	for (int32 Index = 0; Index < VisionTraceSegments; ++Index)
//	{
//		const float Angle = (2.0f * PI * Index) / static_cast<float>(VisionTraceSegments);
//		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
//		FVector TraceEnd = TraceStart + Direction * Source.Radius;
//
//		FHitResult Hit;
//		if (World->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ObjectParams, Params))
//		{
//			TraceEnd = Hit.ImpactPoint - Direction * VisionBlockerProbeBackoff;
//		}
//
//		FVector VisibleWorld = TraceEnd;
//		VisibleWorld.Z = Source.Location.Z;
//		Points.Add(WorldToCanvas(VisibleWorld, Width, Height));
//	}
//
//	const FLinearColor VisibleColor(
//		VisibleFowValue,
//		VisibleFowValue,
//		VisibleFowValue,
//		1.0f
//	);
//
//	for (int32 Index = 0; Index < Points.Num(); ++Index)
//	{
//		const FVector2D& A = Points[Index];
//		const FVector2D& B = Points[(Index + 1) % Points.Num()];
//		AddTriangle(Triangles, CanvasCenter, A, B, VisibleColor);
//	}
//}
//
//FVector2D AVisionManager::WorldToCanvas(const FVector& WorldLocation, int32 Width, int32 Height) const
//{
//	const float SafeWorldWidth = FMath::Max(MapWorldSize.X, KINDA_SMALL_NUMBER);
//	const float SafeWorldHeight = FMath::Max(MapWorldSize.Y, KINDA_SMALL_NUMBER);
//
//	const float U = ((WorldLocation.X - MapCenter.X) / SafeWorldWidth) + 0.5f;
//	const float V = 0.5f - ((WorldLocation.Y - MapCenter.Y) / SafeWorldHeight);
//
//	return FVector2D(U * Width, V * Height);
//}
//
//FVector AVisionManager::CanvasToWorld2D(
//	const FVector2D& CanvasLocation,
//	int32 Width,
//	int32 Height,
//	float Z) const
//{
//	const float U = Width > 0 ? CanvasLocation.X / Width : 0.5f;
//	const float V = Height > 0 ? CanvasLocation.Y / Height : 0.5f;
//
//	return FVector(
//		(U - 0.5f) * MapWorldSize.X + MapCenter.X,
//		(0.5f - V) * MapWorldSize.Y + MapCenter.Y,
//		Z
//	);
//}
//
//void AVisionManager::AddTriangle(
//	TArray<FCanvasUVTri>& Triangles,
//	const FVector2D& A,
//	const FVector2D& B,
//	const FVector2D& C,
//	const FLinearColor& Color) const
//{
//	FCanvasUVTri Tri;
//	Tri.V0_Pos = A;
//	Tri.V1_Pos = B;
//	Tri.V2_Pos = C;
//	Tri.V0_UV = FVector2D::ZeroVector;
//	Tri.V1_UV = FVector2D::UnitVector;
//	Tri.V2_UV = FVector2D(1.0f, 0.0f);
//	Tri.V0_Color = Color;
//	Tri.V1_Color = Color;
//	Tri.V2_Color = Color;
//	Triangles.Add(Tri);
//}
//
//void AVisionManager::ApplyRenderTargetToWorldMaterials()
//{
//	if (!VisionRenderTarget || VisionTextureParameterNames.Num() == 0)
//	{
//		return;
//	}
//
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return;
//	}
//
//	for (TObjectIterator<UMeshComponent> It; It; ++It)
//	{
//		UMeshComponent* MeshComponent = *It;
//		if (!MeshComponent || MeshComponent->GetWorld() != World)
//		{
//			continue;
//		}
//
//		const int32 MaterialCount = MeshComponent->GetNumMaterials();
//		for (int32 Index = 0; Index < MaterialCount; ++Index)
//		{
//			UMaterialInstanceDynamic* MID = MeshComponent->CreateDynamicMaterialInstance(Index);
//			if (!MID)
//			{
//				continue;
//			}
//
//			for (const FName& ParameterName : VisionTextureParameterNames)
//			{
//				MID->SetTextureParameterValue(ParameterName, VisionRenderTarget);
//			}
//		}
//	}
//}
