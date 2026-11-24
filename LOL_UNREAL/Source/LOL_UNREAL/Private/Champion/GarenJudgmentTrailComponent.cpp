#include "GarenJudgmentTrailComponent.h"
#include "Champion/Champion_Garen.h"
#include "Component/LOL_StateComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
    constexpr float TrailLifetime = .19f;
    constexpr int32 MaxSamples = 32;
    const FName WeaponBone(TEXT("Weapon"));
}

UGarenJudgmentTrailComponent::UGarenJudgmentTrailComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    SetCastShadow(false);
}

void UGarenJudgmentTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Garen = CastChecked<AChampion_Garen>(GetOwner());
    const double Now = GetWorld()->GetTimeSeconds();
    Samples.RemoveAll([Now](const FBladeSample& Sample) { return Now - Sample.Time >= TrailLifetime; });
    const bool bEmit = Garen->bIsSpinning && !Garen->StateComponent->HasStatusTag(LOLTags::State_Dead);
    if (bEmit)
    {
        // The blade extends along local -Y from the Weapon bone to the tip at 206.6 cm.
        const FTransform Weapon = Garen->GetMesh()->GetSocketTransform(WeaponBone);
        const FVector Tip = Weapon.TransformPosition(FVector(0., -206.6, 0.));
        const FVector Inner = Weapon.TransformPosition(FVector(0., -48., 0.));
        // Avoid drawing across the map after a teleport or a discontinuous pose update.
        if (!Samples.IsEmpty() && FVector::DistSquared(Tip, Samples.Last().Tip) > FMath::Square(260.f))
        {
            Samples.Reset();
        }
        if (Samples.IsEmpty() || Now - Samples.Last().Time >= .006)
        {
            if (Samples.Num() == MaxSamples) Samples.RemoveAt(0, 1, EAllowShrinking::No);
            Samples.Add({Inner, Tip, Now});
        }
    }

    if (Samples.Num() < 2)
    {
        if (GetNumSections()) ClearAllMeshSections();
        if (!bEmit) SetComponentTickEnabled(false);
        return;
    }

    Vertices.Reset(MaxSamples * 2);
    UVs.Reset(MaxSamples * 2);
    const FTransform& Component = GetComponentTransform();
    for (int32 Index = 0; Index < MaxSamples; ++Index)
    {
        // Collapse unused pairs onto the newest edge to keep one reusable mesh section.
        const FBladeSample& Sample = Samples[FMath::Min(Index, Samples.Num() - 1)];
        const float Age = FMath::Clamp(float((Now - Sample.Time) / TrailLifetime), 0.f, 1.f);
        const FVector Inner = FMath::Lerp(Sample.Inner, Sample.Tip, .52f * Age * Age);
        Vertices.Add(Component.InverseTransformPosition(Inner));
        Vertices.Add(Component.InverseTransformPosition(Sample.Tip));
        UVs.Add(FVector2D(Age, 0.));
        UVs.Add(FVector2D(Age, 1.));
    }

    if (GetNumSections())
    {
        UpdateMeshSection_LinearColor(0, Vertices, {}, UVs, {}, {});
    }
    else
    {
        TArray<int32> Triangles;
        for (int32 I = 0; I < 2 * (MaxSamples - 1); I += 2)
        {
            Triangles.Append({I, I + 1, I + 2, I + 2, I + 1, I + 3});
        }
        CreateMeshSection_LinearColor(0, Vertices, Triangles, {}, UVs, {}, {}, false);
    }
}
