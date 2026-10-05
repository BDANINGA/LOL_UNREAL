#include "Component/LOL_SummonerSpellComponent.h"

#include "BaseChampion.h"
#include "LOL_HUD.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"

ULOL_SummonerSpellComponent::ULOL_SummonerSpellComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULOL_SummonerSpellComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChampion = Cast<ABaseChampion>(GetOwner());
}

void ULOL_SummonerSpellComponent::CastExhaust(AActor* Target)
{
	Server_CastExhaust(Target);
}

void ULOL_SummonerSpellComponent::CastFlash(const FVector& TargetLocation)
{
	Server_CastFlash(TargetLocation);
}

bool ULOL_SummonerSpellComponent::CanCastSummonerSpell() const
{
	if (!OwnerChampion || !OwnerChampion->StateComponent)
	{
		return false;
	}

	return
		!OwnerChampion->StateComponent->HasStatusTag(LOLTags::State_Dead) &&
		!OwnerChampion->bIsStunned &&
		!OwnerChampion->bIsKnockedBack &&
		!OwnerChampion->IsMoveInputBlocked();
}

void ULOL_SummonerSpellComponent::Server_CastExhaust_Implementation(AActor* Target)
{
	ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target);
	if (!CanCastSummonerSpell() || !TargetChampion || TargetChampion == OwnerChampion)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < ExhaustCooldownEndTime ||
		!OwnerChampion->IsEnemyActor(TargetChampion) ||
		!TargetChampion->StateComponent ||
		TargetChampion->StateComponent->HasStatusTag(LOLTags::State_Dead))
	{
		return;
	}

	const float TargetRadius = TargetChampion->GetCapsuleComponent()
		? TargetChampion->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 0.0f;
	const float Distance = FMath::Max(
		0.0f,
		FVector::Dist2D(OwnerChampion->GetActorLocation(), TargetChampion->GetActorLocation()) - TargetRadius);
	if (Distance > ExhaustRange)
	{
		return;
	}

	if (!TargetChampion->SummonerSpellComponent)
	{
		return;
	}

	TargetChampion->SummonerSpellComponent->ApplyExhaustEffect(
		ExhaustDuration,
		ExhaustMoveSpeedMultiplier,
		ExhaustDamageMultiplier);

	ExhaustCooldownEndTime = CurrentTime + ExhaustCooldown;
	Client_StartSummonerSpellCooldown(TEXT("Spell1"), ExhaustCooldown);
}

void ULOL_SummonerSpellComponent::Server_CastFlash_Implementation(FVector_NetQuantize TargetLocation)
{
	if (!CanCastSummonerSpell() || TargetLocation.ContainsNaN())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < FlashCooldownEndTime)
	{
		return;
	}

	const FVector StartLocation = OwnerChampion->GetActorLocation();
	FVector Direction = FVector(TargetLocation) - StartLocation;
	Direction.Z = 0.0f;
	const float RequestedDistance = Direction.Size2D();
	if (RequestedDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Direction /= RequestedDistance;
	FVector Destination = StartLocation + Direction * FMath::Min(RequestedDistance, FlashRange);
	Destination.Z = StartLocation.Z;
	const FRotator FacingRotation(0.0f, Direction.Rotation().Yaw, 0.0f);

	if (!OwnerChampion->TeleportTo(Destination, FacingRotation, false, false))
	{
		return;
	}

	if (OwnerChampion->AttackComponent)
	{
		OwnerChampion->AttackComponent->CancelAttack();
		OwnerChampion->AttackComponent->CombatTarget = nullptr;
		OwnerChampion->AttackComponent->HitTarget = nullptr;
	}
	if (OwnerChampion->MoveComponent)
	{
		OwnerChampion->MoveComponent->StopMovement();
		OwnerChampion->MoveComponent->bIsSearchAttack = false;
	}
	if (UCharacterMovementComponent* Movement = OwnerChampion->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	FlashCooldownEndTime = CurrentTime + FlashCooldown;
	Client_StartSummonerSpellCooldown(TEXT("Spell2"), FlashCooldown);
}

void ULOL_SummonerSpellComponent::ApplyExhaustEffect(
	float Duration,
	float MoveSpeedMultiplier,
	float DamageMultiplier)
{
	if (!OwnerChampion || !OwnerChampion->HasAuthority())
	{
		return;
	}

	UCharacterMovementComponent* Movement = OwnerChampion->GetCharacterMovement();
	if (!bExhaustActive && Movement)
	{
		PreExhaustMoveSpeed = Movement->MaxWalkSpeed;
	}

	bExhaustActive = true;
	OutgoingDamageMultiplier = FMath::Clamp(DamageMultiplier, 0.0f, 1.0f);
	if (Movement)
	{
		ExhaustAppliedMoveSpeed = PreExhaustMoveSpeed * FMath::Clamp(MoveSpeedMultiplier, 0.0f, 1.0f);
		Movement->MaxWalkSpeed = FMath::Min(Movement->MaxWalkSpeed, ExhaustAppliedMoveSpeed);
		Client_SetExhaustMoveSpeed(Movement->MaxWalkSpeed);
	}

	GetWorld()->GetTimerManager().ClearTimer(ExhaustEffectTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ExhaustEffectTimerHandle,
		this,
		&ULOL_SummonerSpellComponent::ClearExhaustEffect,
		FMath::Max(0.01f, Duration),
		false);
}

void ULOL_SummonerSpellComponent::ClearExhaustEffect()
{
	if (!OwnerChampion || !OwnerChampion->HasAuthority())
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = OwnerChampion->GetCharacterMovement())
	{
		if (FMath::IsNearlyEqual(Movement->MaxWalkSpeed, ExhaustAppliedMoveSpeed, 0.1f))
		{
			Movement->MaxWalkSpeed = PreExhaustMoveSpeed;
		}
		Client_SetExhaustMoveSpeed(Movement->MaxWalkSpeed);
	}

	bExhaustActive = false;
	OutgoingDamageMultiplier = 1.0f;
	PreExhaustMoveSpeed = 0.0f;
	ExhaustAppliedMoveSpeed = 0.0f;
}

void ULOL_SummonerSpellComponent::Client_SetExhaustMoveSpeed_Implementation(float MoveSpeed)
{
	if (!OwnerChampion)
	{
		OwnerChampion = Cast<ABaseChampion>(GetOwner());
	}
	if (OwnerChampion && OwnerChampion->IsLocallyControlled())
	{
		if (UCharacterMovementComponent* Movement = OwnerChampion->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = FMath::Max(0.0f, MoveSpeed);
		}
	}
}

void ULOL_SummonerSpellComponent::Client_StartSummonerSpellCooldown_Implementation(
	FName SpellName,
	float CooldownDuration)
{
	if (!OwnerChampion)
	{
		OwnerChampion = Cast<ABaseChampion>(GetOwner());
	}
	if (!OwnerChampion || !OwnerChampion->IsLocallyControlled())
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(OwnerChampion->GetController()))
	{
		if (ALOL_HUD* HUD = Cast<ALOL_HUD>(PlayerController->GetHUD()))
		{
			HUD->UpdateSkillCoolDown(
				SpellName,
				GetWorld()->GetTimeSeconds() + CooldownDuration,
				CooldownDuration);
		}
	}
}
