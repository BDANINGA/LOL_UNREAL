#pragma once

#include "ProceduralMeshComponent.h"
#include "GarenJudgmentTrailComponent.generated.h"

// A short world-space history of the animated greatsword, sampled after the mesh pose updates.
UCLASS()
class UGarenJudgmentTrailComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:
    UGarenJudgmentTrailComponent(const FObjectInitializer& ObjectInitializer);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

private:
    struct FBladeSample
    {
        FVector Inner;
        FVector Tip;
        double Time;
    };

    TArray<FBladeSample> Samples;
    TArray<FVector> Vertices;
    TArray<FVector2D> UVs;
};
