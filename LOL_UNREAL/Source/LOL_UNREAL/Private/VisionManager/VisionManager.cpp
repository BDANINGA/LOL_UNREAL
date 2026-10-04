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
	PrimaryActorTick.bCanEverTick = false;
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

    if (!FoWRenderTarget || !VisionBrushMID)
        return;

    FVector2D MapSize = MapMaxBounds - MapMinBounds;

    if (MapSize.X <= 0.f || MapSize.Y <= 0.f)
        return;

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

    // --------------------------------------------------
    // 3. 아군은 항상 보이게
    // --------------------------------------------------

    if (AllyActors)
    {
        for (AActor* Ally : *AllyActors)
        {
            if (!IsValid(Ally))
                continue;

            Ally->SetActorHiddenInGame(false);
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
                Enemy->SetActorHiddenInGame(false);
                continue;
            }

            bool bVisible = false;

            for (ULOL_VisionComponent* VisionComp : *ActiveVisionComponents)
            {
                if (!IsValid(VisionComp))
                    continue;

                AActor* VisionOwner = VisionComp->GetOwner();

                if (!IsValid(VisionOwner))
                    continue;

                const float Dist = FVector::Dist(
                    VisionOwner->GetActorLocation(),
                    Enemy->GetActorLocation()
                );

                if (Dist <= VisionComp->VisionRadius)
                {
                    bVisible = true;
                    break;
                }
            }

            Enemy->SetActorHiddenInGame(!bVisible);
        }
    }
}