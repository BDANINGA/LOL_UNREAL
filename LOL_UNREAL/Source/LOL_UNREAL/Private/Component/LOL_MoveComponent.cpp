// 이동 관련 컴포넌트
#include "Component/LOL_MoveComponent.h"
#include "BaseChampion.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ULOL_MoveComponent::ULOL_MoveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void ULOL_MoveComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ABaseChampion>(GetOwner());
}

void ULOL_MoveComponent::UpdateMovement(float DeltaTime)
{
    if (!Owner || Owner->bIsDead || Owner->bIsStunned || Owner->bIsKnockedBack) return;

    if (Owner->CombatTarget) return; 

    if (bIsMoving)
    {
        FVector CurrentLocation = Owner->GetActorLocation();
        FVector Direction = TargetLocation - CurrentLocation;
        Direction.Z = 0.f;
        float Distance = Direction.Size();

        if (Distance <= 10.f) {
            StopMovement();
        }
        else {
            Owner->AddMovementInput(Direction.GetSafeNormal(), 1.0f);
        }
    }
}

void ULOL_MoveComponent::SetMoveTarget(FVector NewLocation, AActor* TargetActor)
{
    if (TargetActor && TargetActor != Owner) {
        bIsMoving = false;
    }
    else {
        TargetLocation = NewLocation;
        bIsMoving = true;
    }
}

void ULOL_MoveComponent::StopMovement()
{
    bIsMoving = false;
    if (Owner && Owner->GetCharacterMovement())
    {
        Owner->GetCharacterMovement()->StopMovementImmediately();
    }
}

void ULOL_MoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULOL_MoveComponent, TargetLocation);
    DOREPLIFETIME(ULOL_MoveComponent, bIsMoving);
}
