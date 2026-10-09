#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LOL_FixedKnockbackComponent.generated.h"

// Server-driven displacement with a hard distance limit, used by Vayne E.
UCLASS()
class LOL_UNREAL_API ULOL_FixedKnockbackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULOL_FixedKnockbackComponent();
    void StartKnockback(const FVector& Direction, float Distance, float Duration, float WallStunDuration);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void FinishKnockback();

    FVector StartLocation = FVector::ZeroVector;
    FVector PushDirection = FVector::ZeroVector;
    float PushDistance = 0.f;
    float PushDuration = 0.f;
    float ElapsedTime = 0.f;
    float WallStunTime = 0.f;
    EMovementMode PreviousMovementMode = MOVE_Walking;
    uint8 PreviousCustomMovementMode = 0;
    bool bPushing = false;
};
