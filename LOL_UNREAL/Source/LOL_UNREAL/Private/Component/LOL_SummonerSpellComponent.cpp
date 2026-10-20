#include "Component/LOL_SummonerSpellComponent.h"

#include "BaseChampion.h"
#include "LOL_HUD.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

ULOL_SummonerSpellComponent::ULOL_SummonerSpellComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.05f;
	SetIsReplicatedByDefault(true);
	static ConstructorHelpers::FObjectFinder<UParticleSystem> Departure(TEXT("/Game/VFX/SummonerSpells/P_FlashDeparture.P_FlashDeparture"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> Arrival(TEXT("/Game/VFX/SummonerSpells/P_FlashArrival.P_FlashArrival"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> Exhaust(TEXT("/Game/VFX/SummonerSpells/P_ExhaustAura.P_ExhaustAura"));
	if (Departure.Succeeded()) FlashDepartureEffect = Departure.Object;
	if (Arrival.Succeeded()) FlashArrivalEffect = Arrival.Object;
	if (Exhaust.Succeeded()) ExhaustAuraEffect = Exhaust.Object;
}

void ULOL_SummonerSpellComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChampion = Cast<ABaseChampion>(GetOwner());
	if (ExhaustVisualEndTime > 0.0f) OnRep_ExhaustVisualEndTime();
}

void ULOL_SummonerSpellComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULOL_SummonerSpellComponent, ExhaustVisualEndTime);
}

float ULOL_SummonerSpellComponent::GetSpellServerTime() const
{
	const AGameStateBase* GS = GetWorld()->GetGameState();
	return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

void ULOL_SummonerSpellComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(ExhaustEffectTimerHandle);
	StopExhaustVisual();
	Super::EndPlay(EndPlayReason);
}

void ULOL_SummonerSpellComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateExhaustVisual();
}

void ULOL_SummonerSpellComponent::StopExhaustVisual()
{
	if (ExhaustAuraComponent)
	{
		ExhaustAuraComponent->DestroyComponent();
		ExhaustAuraComponent = nullptr;
	}
	SetComponentTickEnabled(false);
}

void ULOL_SummonerSpellComponent::OnRep_ExhaustVisualEndTime()
{
	OwnerChampion = Cast<ABaseChampion>(GetOwner());
	UpdateExhaustVisual();
}

void ULOL_SummonerSpellComponent::UpdateExhaustVisual()
{
	if (GetNetMode() == NM_DedicatedServer || !OwnerChampion || !bEnableSpellEffects
		|| ExhaustVisualEndTime <= GetSpellServerTime()
		|| (OwnerChampion->StateComponent && OwnerChampion->StateComponent->HasStatusTag(LOLTags::State_Dead)))
	{
		StopExhaustVisual();
		return;
	}
	if (!ExhaustAuraComponent && ExhaustAuraEffect)
	{
		ExhaustAuraComponent = UGameplayStatics::SpawnEmitterAttached(ExhaustAuraEffect,
			OwnerChampion->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			FVector::OneVector, EAttachLocation::KeepRelativeOffset, false);
	}
	if (ExhaustAuraComponent)
	{
		// Fog-of-war visibility must not reveal a hidden opponent through its VFX.
		ExhaustAuraComponent->SetVisibility(!OwnerChampion->IsHidden());
		SetComponentTickEnabled(true);
	}
}

void ULOL_SummonerSpellComponent::Multicast_PlayFlashEffect_Implementation(FVector_NetQuantize Departure, FVector_NetQuantize Arrival)
{
	if (GetNetMode() == NM_DedicatedServer || !bEnableSpellEffects || !GetOwner() || GetOwner()->IsHidden()) return;
	if (FlashDepartureEffect)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FlashDepartureEffect, Departure, FRotator::ZeroRotator, FVector::OneVector, true);
	if (FlashArrivalEffect)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FlashArrivalEffect, Arrival, FRotator::ZeroRotator, FVector::OneVector, true);
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
	Multicast_PlayFlashEffect(StartLocation, OwnerChampion->GetActorLocation());

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
	ExhaustVisualEndTime = GetSpellServerTime() + FMath::Max(0.01f, Duration);
	OnRep_ExhaustVisualEndTime();
	OwnerChampion->ForceNetUpdate();
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
	ExhaustVisualEndTime = 0.0f;
	OnRep_ExhaustVisualEndTime();
	OwnerChampion->ForceNetUpdate();
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
