#include "VisionManager/VisionManager.h"

#include "Building/BaseBuilding.h"
#include "BaseChampion.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/WidgetComponent.h"

#include "Component/LOL_VisionComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AVisionManager::AVisionManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVisionManager::BeginPlay()
{
	Super::BeginPlay();
    RebuildBushRegions();
    // Initial registration also covers actors whose replicated tags arrived before this manager.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        RegisterActor(*It);
        if (auto* Vision = It->FindComponentByClass<ULOL_VisionComponent>())
            RegisterVisionComponent(Vision);
    }
	if (VisionBrushMaterial)
	{
		VisionBrushMID = UMaterialInstanceDynamic::Create(VisionBrushMaterial, this);
	}
}

void AVisionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVisionManager::RegisterVisionComponent(ULOL_VisionComponent* Component)
{
    if (!IsValid(Component))
        return;

    ULOL_StateComponent* State =
        Component->GetOwner()
        ? Component->GetOwner()->FindComponentByClass<ULOL_StateComponent>()
        : nullptr;

    if (!State)
    {
        return;
    }

    BlueVisionComponents.Remove(Component);
    RedVisionComponents.Remove(Component);

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
	if(!IsValid(Actor))
        return;

    ULOL_StateComponent* State =
        Actor->FindComponentByClass<ULOL_StateComponent>();

    if (!State)
        return;

    BlueActors.Remove(Actor);
    RedActors.Remove(Actor);

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

    UWorld* World = GetWorld();

    if (!World)
        return;

    APlayerController* LocalPC =
        UGameplayStatics::GetPlayerController(World, 0);

    if (!LocalPC)
        return;

    if (!LocalPC->IsLocalController())
        return;

    const bool bCanDrawFog = FoWRenderTarget && VisionBrushMID;

    FVector2D MapSize = MapMaxBounds - MapMinBounds;

    const bool bValidMapSize = MapSize.X > 0.f && MapSize.Y > 0.f;

    AActor* LocalPlayerPawn = LocalPC->GetPawn();

    if (!LocalPlayerPawn)
        return;

    ULOL_StateComponent* LocalPlayerState =
        LocalPlayerPawn->FindComponentByClass<ULOL_StateComponent>();

    if (!LocalPlayerState)
        return;

    const TArray<ULOL_VisionComponent*>* ActiveVisionComponents = nullptr;
    const TArray<AActor*>* AllyActors = nullptr;
    const TArray<AActor*>* EnemyActors = nullptr;

    if (LocalPlayerState->HasStatusTag(LOLTags::Team_Blue))
    {
        ActiveVisionComponents = &BlueVisionComponents;
        AllyActors = &BlueActors;
        EnemyActors = &RedActors;
    }
    else if (LocalPlayerState->HasStatusTag(LOLTags::Team_Red))
    {

        ActiveVisionComponents = &RedVisionComponents;
        AllyActors = &RedActors;
        EnemyActors = &BlueActors;
    }

    if (!ActiveVisionComponents)
        return;

    // --------------------------------------------------
    // 1. Fog of War RenderTarget 초기화
    // --------------------------------------------------

    if (bCanDrawFog && bValidMapSize)
    UKismetRenderingLibrary::ClearRenderTarget2D(
        this,
        FoWRenderTarget,
        FLinearColor::Black
    );

    // --------------------------------------------------
    // 2. 현재 팀의 시야 그리기
    // --------------------------------------------------

    for (ULOL_VisionComponent* VisionComp : *ActiveVisionComponents)
    {
        if (!IsValid(VisionComp))
            continue;

        AActor* VisionOwner = VisionComp->GetOwner();

        if (!IsValid(VisionOwner))
            continue;

        const auto* SourceState = VisionOwner->FindComponentByClass<ULOL_StateComponent>();
        if (!bCanDrawFog || !bValidMapSize || !SourceState || SourceState->HasStatusTag(LOLTags::State_Dead))
            continue;

        FVector WorldLoc = VisionOwner->GetActorLocation();

        float U =
            (WorldLoc.X - MapMinBounds.X) / MapSize.X;

        float V =
            (WorldLoc.Y - MapMinBounds.Y) / MapSize.Y;

        U = FMath::Clamp(U, 0.f, 1.f);
        V = FMath::Clamp(V, 0.f, 1.f);

        VisionBrushMID->SetVectorParameterValue(
            FName("DrawPosition"),
            FLinearColor(U, V, 0.f, 1.f)
        );

        float RadiusUV =
            VisionComp->VisionRadius /
            FMath::Max(MapSize.X, MapSize.Y);

        VisionBrushMID->SetScalarParameterValue(
            FName("VisionRadius"),
            RadiusUV
        );

        UKismetRenderingLibrary::DrawMaterialToRenderTarget(
            this,
            FoWRenderTarget,
            VisionBrushMID
        );
    }

    auto ApplyLocalVisibility = [this, LocalPC](AActor* Actor, bool bVisible)
    {
        if (bVisible) LocalPC->HiddenActors.Remove(Actor);
        else LocalPC->HiddenActors.AddUnique(Actor);
        if (auto* Champion = Cast<ABaseChampion>(Actor)) Champion->bVisibleByVision = bVisible;
        // Screen-space health bars are not filtered by the renderer's HiddenActors list.
        TInlineComponentArray<UWidgetComponent*> Widgets(Actor);
        for (auto* Widget : Widgets)
        {
            if (bVisible)
            {
                if (const bool* WasVisible = HiddenWidgetVisibility.Find(Widget))
                {
                    Widget->SetVisibility(*WasVisible);
                    HiddenWidgetVisibility.Remove(Widget);
                }
            }
            else
            {
                if (!HiddenWidgetVisibility.Contains(Widget)) HiddenWidgetVisibility.Add(Widget, Widget->IsVisible());
                Widget->SetVisibility(false);
            }
        }
    };
    for (auto It = HiddenWidgetVisibility.CreateIterator(); It; ++It)
        if (!It.Key().IsValid()) It.RemoveCurrent();

    // --------------------------------------------------
    // 3. 아군은 항상 보이게
    // --------------------------------------------------

    if (AllyActors)
    {
        for (AActor* Ally : *AllyActors)
        {
            if (!IsValid(Ally))
                continue;

            ApplyLocalVisibility(Ally, true);
        }
    }

    // --------------------------------------------------
    // 4. 적군은 시야 안에서만 보이게
    // --------------------------------------------------

    if (EnemyActors)
    {
        for (AActor* Enemy : *EnemyActors)
        {
            if (!IsValid(Enemy))
                continue;

            if (Cast<ABaseBuilding>(Enemy))
            {
                ApplyLocalVisibility(Enemy, true);
                continue;
            }

            const bool bVisible = CanTeamSeeActor(LocalPlayerPawn, Enemy);
            // Per-view hiding avoids replicating the listen server's bHidden to all clients.
            ApplyLocalVisibility(Enemy, bVisible);
        }
    }
}

void AVisionManager::SetBushRegions(const TArray<FBox>& Regions)
{
    BushRegions = Regions;
    BushGroups.SetNum(Regions.Num());
    for (int32 I = 0; I < Regions.Num(); ++I) BushGroups[I] = I;
    // Touching plants form one bush, but retain their individual footprints (no empty-gap hiding).
    for (int32 I = 0; I < Regions.Num(); ++I)
    {
        for (int32 J = 0; J < I; ++J)
        {
            if (!Regions[I].Intersect(Regions[J])) continue;
            const int32 OldGroup = BushGroups[I];
            const int32 NewGroup = BushGroups[J];
            if (OldGroup == NewGroup) continue;
            for (int32& Group : BushGroups) if (Group == OldGroup) Group = NewGroup;
        }
    }
}

void AVisionManager::RebuildBushRegions()
{
    TArray<FBox> Regions;
    if (!BushMeshName.IsEmpty())
    {
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            TInlineComponentArray<UStaticMeshComponent*> Components(*It);
            for (const auto* Component : Components)
            {
                const UStaticMesh* Mesh = Component->GetStaticMesh();
                if (!Mesh || !Mesh->GetName().Equals(BushMeshName, ESearchCase::IgnoreCase)) continue;
                const auto* Instances = Cast<UInstancedStaticMeshComponent>(Component);
                const int32 Count = Instances ? Instances->GetInstanceCount() : 1;
                for (int32 Index = 0; Index < Count; ++Index)
                {
                    FTransform Transform = Component->GetComponentTransform();
                    if (Instances && !Instances->GetInstanceTransform(Index, Transform, true)) continue;
                    const FBox Bounds = Mesh->GetBoundingBox().TransformBy(Transform);
                    FVector Center = Bounds.GetCenter();
                    FVector Extent = Bounds.GetExtent();
                    Extent.X *= BushFootprintScale;
                    Extent.Y *= BushFootprintScale;
                    Center.Z = Bounds.Min.Z + BushHeight * 0.5f;
                    Extent.Z = BushHeight * 0.5f;
                    Regions.Add(FBox(Center - Extent, Center + Extent));
                }
            }
        }
    }
    SetBushRegions(Regions);
    UE_LOG(LogTemp, Log, TEXT("VisionManager: %d bush footprints from %s"), Regions.Num(), *BushMeshName);
}

bool AVisionManager::CanSourceSeeActor(const ULOL_VisionComponent* Source, const AActor* Target) const
{
    if (!IsValid(Source) || !IsValid(Target) || !IsValid(Source->GetOwner())) return false;
    const AActor* SourceActor = Source->GetOwner();
    const auto* State = SourceActor->FindComponentByClass<ULOL_StateComponent>();
    if (!State || State->HasStatusTag(LOLTags::State_Dead)) return false;
    if (FVector::DistSquared(SourceActor->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(Source->VisionRadius)) return false;

    bool bTargetInBush = false;
    for (int32 I = 0; I < BushRegions.Num(); ++I)
    {
        if (!BushRegions[I].IsInsideOrOn(Target->GetActorLocation())) continue;
        bTargetInBush = true;
        for (int32 J = 0; J < BushRegions.Num(); ++J)
        {
            if (BushGroups[I] == BushGroups[J] && BushRegions[J].IsInsideOrOn(SourceActor->GetActorLocation())) return true;
        }
    }
    return !bTargetInBush;
}

bool AVisionManager::CanTeamSeeActor(const AActor* Viewer, const AActor* Target) const
{
    if (!IsValid(Viewer) || !IsValid(Target)) return false;
    const auto* ViewerState = Viewer->FindComponentByClass<ULOL_StateComponent>();
    const auto* TargetState = Target->FindComponentByClass<ULOL_StateComponent>();
    if (!ViewerState || !TargetState) return true;
    if (!ViewerState->IsEnemy(const_cast<ULOL_StateComponent*>(TargetState)) || Cast<ABaseBuilding>(Target)) return true;
    // Neutral monsters retain their existing targeting rules.
    if (!TargetState->HasStatusTag(LOLTags::Team_Blue) && !TargetState->HasStatusTag(LOLTags::Team_Red)) return true;
    const TArray<ULOL_VisionComponent*>* Sources = nullptr;
    if (ViewerState->HasStatusTag(LOLTags::Team_Blue)) Sources = &BlueVisionComponents;
    else if (ViewerState->HasStatusTag(LOLTags::Team_Red)) Sources = &RedVisionComponents;
    if (!Sources) return true;
    for (const auto* Source : *Sources)
        if (CanSourceSeeActor(Source, Target)) return true;
    return false;
}
