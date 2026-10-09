#include "Component/LOL_FixedKnockbackComponent.h"

#include "BaseChampion.h"
#include "Building/BaseBuilding.h"
#include "Component/LOL_StateComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "JungleMonster/BaseJungleMonster.h"

ULOL_FixedKnockbackComponent::ULOL_FixedKnockbackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void ULOL_FixedKnockbackComponent::StartKnockback(
    const FVector& Direction, float Distance, float Duration, float WallStunDuration)
{
    ACharacter* Target = Cast<ACharacter>(GetOwner());
    if (!Target || !Target->HasAuthority() || !Target->GetCharacterMovement()) return;

    FinishKnockback();
    PushDirection = Direction.GetSafeNormal2D();
    if (PushDirection.IsNearlyZero() || Distance <= 0.f) return;

    UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
    StartLocation = Target->GetActorLocation();
    PushDistance = Distance;
    PushDuration = FMath::Max(Duration, 0.01f);
    WallStunTime = WallStunDuration;
    ElapsedTime = 0.f;
    PreviousMovementMode = Movement->MovementMode;
    PreviousCustomMovementMode = Movement->CustomMovementMode;

    if (ABaseChampion* Champion = Cast<ABaseChampion>(Target))
    {
        Champion->EndKnockback();
        Champion->SetIsKnockedBack(true);
    }
    if (ABaseJungleMonster* Monster = Cast<ABaseJungleMonster>(Target))
    {
        Monster->ApplyCrowdControl(PushDuration);
    }
    Target->StopAnimMontage();
    Movement->ClearAccumulatedForces();
    Movement->StopMovementImmediately();
    Movement->CurrentRootMotion.Clear();
    Movement->DisableMovement();
    bPushing = true;
    SetComponentTickEnabled(true);
}

void ULOL_FixedKnockbackComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ACharacter* Target = Cast<ACharacter>(GetOwner());
    if (!bPushing || !Target || !Target->HasAuthority()) return;
    if (const ULOL_StateComponent* State = Target->FindComponentByClass<ULOL_StateComponent>())
    {
        if (State->HasStatusTag(LOLTags::State_Dead))
        {
            FinishKnockback();
            return;
        }
    }

    ElapsedTime = FMath::Min(ElapsedTime + FMath::Max(DeltaTime, 0.f), PushDuration);
    const float Alpha = ElapsedTime / PushDuration;
    FHitResult Hit;
    Target->SetActorLocation(StartLocation + PushDirection * (PushDistance * Alpha), true, &Hit);
    Target->ForceNetUpdate();

    if (Hit.bBlockingHit)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        const bool bHitWall = Cast<ABaseBuilding>(Hit.GetActor()) != nullptr ||
            (HitComponent && HitComponent->GetCollisionObjectType() == ECC_WorldStatic &&
                FMath::Abs(Hit.ImpactNormal.Z) < 0.5f);
        FinishKnockback();
        if (bHitWall)
        {
            if (ABaseChampion* Champion = Cast<ABaseChampion>(Target))
            {
                Champion->Multicast_ApplyStun(WallStunTime);
            }
        }
    }
    else if (Alpha >= 1.f)
    {
        FinishKnockback();
    }
}

void ULOL_FixedKnockbackComponent::FinishKnockback()
{
    if (!bPushing) return;
    bPushing = false;
    SetComponentTickEnabled(false);
    if (ACharacter* Target = Cast<ACharacter>(GetOwner()))
    {
        UCharacterMovementComponent* Movement = Target->GetCharacterMovement();
        Movement->ClearAccumulatedForces();
        Movement->StopMovementImmediately();
        Movement->SetMovementMode(PreviousMovementMode, PreviousCustomMovementMode);
        if (ABaseChampion* Champion = Cast<ABaseChampion>(Target))
        {
            Champion->SetIsKnockedBack(false);
        }
        Target->ForceNetUpdate();
    }
}
