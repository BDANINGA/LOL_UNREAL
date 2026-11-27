#include "Champion/Projectile/VayneCondemnProjectile.h"
#include "Champion/Champion_Vayne.h"
#include "Component/LOL_StateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AVayneCondemnProjectile::AVayneCondemnProjectile()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Bolt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CondemnBolt"));
    Bolt->SetupAttachment(RootComponent);
    Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bolt->SetGenerateOverlapEvents(false);
    Bolt->SetCanEverAffectNavigation(false);
    Bolt->SetCastShadow(false);
    Bolt->bReceivesDecals = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Game/VFX/Vayne/SM_VayneCondemnBolt.SM_VayneCondemnBolt"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/VFX/Vayne/M_VayneCondemnGlow.M_VayneCondemnGlow"));
    Bolt->SetStaticMesh(Mesh.Object);
    Plane = PlaneAsset.Object;
    Glow = Material.Object;
}

float AVayneCondemnProjectile::ServerTime() const
{
    const AGameStateBase* State = GetWorld()->GetGameState();
    return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void AVayneCondemnProjectile::Initialize(AChampion_Vayne* Shooter, ACharacter* InTarget)
{
    if (!HasAuthority() || !IsValid(Shooter) || !IsValid(InTarget)) return;
    SetOwner(Shooter);
    Target = InTarget;
    LaunchOrigin = GetActorLocation();
    LaunchServerTime = ServerTime();
    FlightDuration = FMath::Clamp(FVector::Dist2D(LaunchOrigin, Target->GetActorLocation()) / 2200.f, .09f, .24f);
    PushDirection = (Target->GetActorLocation() - Shooter->GetActorLocation()).GetSafeNormal2D();
    SetLifeSpan(2.f);
    ForceNetUpdate();
}

void AVayneCondemnProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) return;
    for (int32 Index = 0; Index < 16; ++Index)
    {
        auto* Layer = NewObject<UStaticMeshComponent>(this);
        Layer->SetupAttachment(RootComponent);
        Layer->SetStaticMesh(Plane);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCanEverAffectNavigation(false);
        Layer->SetCastShadow(false);
        Layer->bReceivesDecals = false;
        Layer->RegisterComponent();
        GlowLayers.Add(Layer);
        Materials.Add(Layer->CreateDynamicMaterialInstance(0, Glow));
    }
    bVisualsReady = true;
    OnRep_Impact();
    UpdateVisual(0.f);
}

void AVayneCondemnProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    LocalAge += DeltaSeconds;
    if (Phase == 0)
    {
        const ULOL_StateComponent* State = IsValid(Target) ? Target->FindComponentByClass<ULOL_StateComponent>() : nullptr;
        if (!IsValid(Target) || (State && State->HasStatusTag(LOLTags::State_Dead)))
        {
            // Unresolved references on a client may arrive in the next packet.
            SetActorHiddenInGame(true);
            if (HasAuthority()) Destroy();
            return;
        }
        SetActorHiddenInGame(false);
        const float Alpha = FMath::Clamp((ServerTime() - LaunchServerTime) / FlightDuration, 0.f, 1.f);
        const FVector Aim = Target->GetActorLocation();
        const FVector Direction = (Aim - LaunchOrigin).GetSafeNormal();
        SetActorLocationAndRotation(FMath::Lerp(LaunchOrigin, Aim, Alpha), Direction.Rotation());
        if (HasAuthority() && Alpha >= 1.f)
        {
            // Change phase before damage so no subsequent tick can apply a second hit.
            BeginImpact(1, Aim);
            if (AChampion_Vayne* Shooter = Cast<AChampion_Vayne>(GetOwner()))
                Shooter->ApplyCondemnHit(Target, PushDirection);
        }
    }
    UpdateVisual(DeltaSeconds);
}

void AVayneCondemnProjectile::BeginImpact(uint8 InPhase, FVector Position)
{
    Phase = InPhase;
    ImpactPosition = Position;
    OnRep_Impact();
    SetLifeSpan(InPhase == 2 ? .65f : .4f);
    ForceNetUpdate();
}

void AVayneCondemnProjectile::SpawnWallImpact(UWorld* World, FVector Position, FVector Normal)
{
    if (!World || World->GetNetMode() == NM_Client) return;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (auto* Effect = World->SpawnActor<AVayneCondemnProjectile>(Position, Normal.Rotation(), Params))
        Effect->BeginImpact(2, Position);
}

void AVayneCondemnProjectile::OnRep_Impact()
{
    if (Phase == 0) return;
    SetActorLocation(ImpactPosition);
    if (VisualPhase != Phase)
    {
        VisualPhase = Phase;
        ImpactAge = 0.f;
    }
    Bolt->SetVisibility(false);
}

void AVayneCondemnProjectile::UpdateVisual(float DeltaSeconds)
{
    if (!bVisualsReady) return;
    if (Phase != 0) ImpactAge += DeltaSeconds;
    const float Duration = Phase == 2 ? .58f : .32f;
    const float Fade = FMath::Square(FMath::Clamp(1.f - ImpactAge / Duration, 0.f, 1.f));
    for (int32 Index = 0; Index < GlowLayers.Num(); ++Index)
    {
        UStaticMeshComponent* Layer = GlowLayers[Index];
        FLinearColor Tint;
        float Brightness = 1.f;
        if (Phase == 0)
        {
            Layer->SetVisibility(Index < 4);
            if (Index >= 4) continue;
            // Crossed tapered streaks and a narrow white core behind the silver tip.
            Layer->SetRelativeLocation(FVector(Index < 2 ? -94.f : -37.f, 0.f, 0.f));
            Layer->SetRelativeRotation(FRotator(0.f, 0.f, Index % 2 ? 90.f : 0.f));
            Layer->SetRelativeScale3D(Index < 2 ? FVector(2.4f,.30f,1.f) : FVector(1.55f,.065f,1.f));
            Tint = Index < 2 ? FLinearColor(.38f,.055f,.8f) : FLinearColor(.75f,.8f,1.f);
            Brightness = .85f + .15f * FMath::Sin(LocalAge * 40.f);
        }
        else
        {
            Layer->SetVisibility(Fade > .001f);
            const float Size = Phase == 2 ? 1.65f : 1.f;
            const float Angle = Index * 2.399963f;
            const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), (Index%3-1)*.48f);
            const FVector Direction = Radial.GetSafeNormal();
            const float Radius = (12.f + ImpactAge * (170.f + Index%4*40.f))*Size;
            Layer->SetWorldLocation(ImpactPosition + Direction * Radius);
            Layer->SetWorldRotation(Direction.Rotation());
            Layer->SetWorldScale3D(FVector((.38f+.12f*(Index%3))*Size*(.4f+.6f*Fade), .045f*Size, 1.f));
            Tint = Index%3 == 0 ? FLinearColor(.5f,.09f,1.f) : FLinearColor(.8f,.86f,1.f);
            Brightness = Fade * (Phase == 2 ? 1.8f : 1.2f);
            // A brief contact flash anchors the outward sparks.
            if (Index == 0)
            {
                Layer->SetWorldLocation(ImpactPosition);
                Layer->SetWorldRotation(FRotator::ZeroRotator);
                Layer->SetWorldScale3D(FVector((.45f+ImpactAge*2.f)*Size));
                Brightness = Fade * 1.4f;
            }
        }
        if (Materials[Index])
        {
            Materials[Index]->SetVectorParameterValue(TEXT("Tint"), Tint);
            Materials[Index]->SetScalarParameterValue(TEXT("Fade"), Brightness);
        }
    }
}

void AVayneCondemnProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVayneCondemnProjectile, Target);
    DOREPLIFETIME(AVayneCondemnProjectile, LaunchOrigin);
    DOREPLIFETIME(AVayneCondemnProjectile, FlightDuration);
    DOREPLIFETIME(AVayneCondemnProjectile, LaunchServerTime);
    DOREPLIFETIME(AVayneCondemnProjectile, ImpactPosition);
    DOREPLIFETIME(AVayneCondemnProjectile, Phase);
}
