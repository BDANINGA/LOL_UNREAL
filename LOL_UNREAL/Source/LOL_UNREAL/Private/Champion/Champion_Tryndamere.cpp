#include "Champion/Champion_Tryndamere.h"

#include "Animation/AnimMontage.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AChampion_Tryndamere::AChampion_Tryndamere()
{
	ChampionName = TEXT("Tryndamere");
	SetChampionData(ChampionName);
}

void AChampion_Tryndamere::Skill_Q()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
	Server_Skill_Q();
}

void AChampion_Tryndamere::Skill_W()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
	Server_Skill_W();
}

void AChampion_Tryndamere::Skill_E()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack || bESpinning) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FVector TargetLocation =
		GetActorLocation() + GetActorForwardVector() * 660.0f;
	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		TargetLocation = Hit.ImpactPoint;
	}

	Server_Skill_E(TargetLocation);
}

void AChampion_Tryndamere::Skill_R()
{
	if (!IsLocallyControlled()) return;
	Server_Skill_R();
}

bool AChampion_Tryndamere::Server_Skill_Q_Validate()
{
	return true;
}

void AChampion_Tryndamere::Server_Skill_Q_Implementation()
{
	if (!SkillComponent || !StatComponent) return;

	const FSkillData& QData = SkillComponent->GetQ_Data();
	if (!HasCastData(QData) || !SkillComponent->TryCastSkill("Q", 1)) return;

	BeginMovementLock(GetAnimationDuration(0, 0.45f));
	Multicast_PlayTryndamereSkillAnimation(0, 1.0f, GetActorRotation());

	const float BaseHeal = GetSkillValue(QData.BaseDamage, 0, 30.0f);
	const float MissingHP =
		FMath::Max(
			0.0f,
			StatComponent->GetStat().MaxHP -
				StatComponent->GetCurrentHP()
		);
	const float HealAmount =
		BaseHeal +
		Fury * QHealPerFury +
		StatComponent->GetStat().AbilityPower * QHealAbilityPowerRatio +
		MissingHP * QMissingHealthHealRatio;

	StatComponent->SetHP(StatComponent->GetCurrentHP() + HealAmount);
	Fury = 0.0f;
}

bool AChampion_Tryndamere::Server_Skill_W_Validate()
{
	return true;
}

void AChampion_Tryndamere::Server_Skill_W_Implementation()
{
	if (!SkillComponent || !StatComponent) return;

	const FSkillData& WData = SkillComponent->GetW_Data();
	if (!HasCastData(WData) || !SkillComponent->TryCastSkill("W", 1)) return;

	BeginMovementLock(GetAnimationDuration(1, 0.45f));
	Multicast_PlayTryndamereSkillAnimation(1, 1.0f, GetActorRotation());

	const float Radius = GetSkillValue(WData.Range, 0, WRadius);
	const float Duration = GetSkillValue(WData.Duration, 0, WDebuffDuration);
	const float BaseReduction = GetSkillValue(WData.BaseDamage, 0, 20.0f);

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		GetActorLocation(),
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);
	if (!bHit) return;

	TSet<TWeakObjectPtr<ABaseChampion>> DebuffedTargets;
	for (const FHitResult& Hit : Hits)
	{
		ABaseChampion* Target = Cast<ABaseChampion>(Hit.GetActor());
		if (!IsValid(Target) || Target == this || DebuffedTargets.Contains(Target)) continue;
		if (!Target->StatComponent) continue;

		DebuffedTargets.Add(Target);
		const TWeakObjectPtr<ABaseChampion> TargetKey(Target);

		if (FTimerHandle* ExistingTimer = WDebuffTimers.Find(TargetKey))
		{
			GetWorldTimerManager().ClearTimer(*ExistingTimer);
			EndWDebuff(TargetKey);
		}

		FChampionStat TargetStat = Target->StatComponent->GetStat();
		const float AttackDamageReduction =
			BaseReduction +
			TargetStat.AttackDamage * WAttackDamageReductionRatio;
		TargetStat.AttackDamage =
			FMath::Max(0.0f, TargetStat.AttackDamage - AttackDamageReduction);
		Target->StatComponent->SetStat(TargetStat);
		WAttackDamageReductions.Add(TargetKey, AttackDamageReduction);

		if (UCharacterMovementComponent* TargetMovement = Target->GetCharacterMovement())
		{
			const float OriginalMoveSpeed = TargetMovement->MaxWalkSpeed;
			WOriginalMoveSpeeds.Add(TargetKey, OriginalMoveSpeed);
			TargetMovement->MaxWalkSpeed =
				OriginalMoveSpeed * (1.0f - FMath::Clamp(WSlowRatio, 0.0f, 0.9f));
		}

		FTimerHandle& DebuffTimer = WDebuffTimers.FindOrAdd(TargetKey);
		FTimerDelegate RestoreDelegate;
		RestoreDelegate.BindUObject(
			this,
			&AChampion_Tryndamere::EndWDebuff,
			TargetKey
		);
		GetWorldTimerManager().SetTimer(
			DebuffTimer,
			RestoreDelegate,
			FMath::Max(Duration, 0.1f),
			false
		);
	}
}

bool AChampion_Tryndamere::Server_Skill_E_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_Tryndamere::Server_Skill_E_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bESpinning) return;

	const FSkillData& EData = SkillComponent->GetE_Data();
	if (!HasCastData(EData) || !SkillComponent->TryCastSkill("E", 1)) return;

	const float Range = GetSkillValue(EData.Range, 0, 660.0f);
	EStartLocation = GetActorLocation();
	ETargetLocation = ClampTargetLocation(TargetLocation, Range);

	FVector Direction = ETargetLocation - EStartLocation;
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;

	if (MoveComponent)
	{
		MoveComponent->StopMovement();
	}
	if (AttackComponent)
	{
		AttackComponent->CancelAttack();
		AttackComponent->CombatTarget = nullptr;
		AttackComponent->HitTarget = nullptr;
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	bESpinning = true;
	EElapsed = 0.0f;
	EHitTargets.Empty();
	Multicast_PlayTryndamereSkillAnimation(2, 1.0f, Direction.Rotation());
}

bool AChampion_Tryndamere::Server_Skill_R_Validate()
{
	return true;
}

void AChampion_Tryndamere::Server_Skill_R_Implementation()
{
	if (!SkillComponent || !StatComponent) return;

	const FSkillData& RData = SkillComponent->GetR_Data();
	if (!HasCastData(RData) || !SkillComponent->TryCastSkill("R", 1)) return;

	BeginMovementLock(GetAnimationDuration(3, 0.45f));
	Multicast_PlayTryndamereSkillAnimation(3, 1.0f, GetActorRotation());

	const float Duration = GetSkillValue(RData.Duration, 0, RDuration);
	const float MinimumHP = GetSkillValue(RData.SecondaryValue, 0, RMinimumHP);

	RMinimumHP = FMath::Max(1.0f, MinimumHP);
	bUndyingRageActive = true;
	StatComponent->SetHP(FMath::Max(StatComponent->GetCurrentHP(), RMinimumHP));

	GetWorldTimerManager().ClearTimer(RTimerHandle);
	GetWorldTimerManager().SetTimer(
		RTimerHandle,
		this,
		&AChampion_Tryndamere::EndUndyingRage,
		FMath::Max(Duration, 0.1f),
		false
	);
}

void AChampion_Tryndamere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bESpinning)
	{
		UpdateESpin(DeltaTime);
	}
}

void AChampion_Tryndamere::UpdateESpin(float DeltaTime)
{
	if (!SkillComponent || !StatComponent)
	{
		FinishESpin();
		return;
	}

	EElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		EElapsed / FMath::Max(EDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	const FVector CurrentLocation = GetActorLocation();
	const FVector DesiredLocation =
		FMath::Lerp(EStartLocation, ETargetLocation, Alpha);

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		CurrentLocation,
		DesiredLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(EHitRadius),
		QueryParams
	);

	if (bHit)
	{
		const FSkillData& EData = SkillComponent->GetE_Data();
		const float BaseDamage = GetSkillValue(EData.BaseDamage, 0, 80.0f);
		const float SkillDamage =
			BaseDamage +
			StatComponent->GetStat().BonusAttackDamage * EBonusADRatio +
			StatComponent->GetStat().AbilityPower * EAbilityPowerRatio;

		for (const FHitResult& Hit : Hits)
		{
			ABaseChampion* Target = Cast<ABaseChampion>(Hit.GetActor());
			if (!IsValid(Target) || Target == this || EHitTargets.Contains(Target)) continue;

			EHitTargets.Add(Target);
			UGameplayStatics::ApplyDamage(
				Target,
				SkillDamage,
				GetController(),
				this,
				ULOL_DamagePhysical::StaticClass()
			);
			Fury = FMath::Clamp(Fury + FuryOnSkillHit, 0.0f, MaxFury);
		}
	}

	SetActorLocation(DesiredLocation, true);

	if (Alpha >= 1.0f)
	{
		FinishESpin();
	}
}

void AChampion_Tryndamere::FinishESpin()
{
	if (!bESpinning) return;

	bESpinning = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AChampion_Tryndamere::OnBasicAttackHit(ACharacter* Target)
{
	if (!HasAuthority() || !IsValid(Target) || Target == this) return;
	Fury = FMath::Clamp(Fury + FuryOnHit, 0.0f, MaxFury);
}

void AChampion_Tryndamere::EndWDebuff(TWeakObjectPtr<ABaseChampion> Target)
{
	if (Target.IsValid() && Target->StatComponent)
	{
		FChampionStat TargetStat = Target->StatComponent->GetStat();
		const float* Reduction = WAttackDamageReductions.Find(Target);
		if (Reduction)
		{
			TargetStat.AttackDamage += *Reduction;
			Target->StatComponent->SetStat(TargetStat);
		}

		if (UCharacterMovementComponent* TargetMovement = Target->GetCharacterMovement())
		{
			if (const float* OriginalMoveSpeed = WOriginalMoveSpeeds.Find(Target))
			{
				TargetMovement->MaxWalkSpeed = *OriginalMoveSpeed;
			}
			else
			{
				const float BaseMoveSpeed =
					TargetStat.MoveSpeed > 0.0f ? TargetStat.MoveSpeed : 330.0f;
				TargetMovement->MaxWalkSpeed = BaseMoveSpeed;
			}
		}
	}

	if (FTimerHandle* ExistingTimer = WDebuffTimers.Find(Target))
	{
		GetWorldTimerManager().ClearTimer(*ExistingTimer);
	}
	WDebuffTimers.Remove(Target);
	WAttackDamageReductions.Remove(Target);
	WOriginalMoveSpeeds.Remove(Target);
}

void AChampion_Tryndamere::EndUndyingRage()
{
	bUndyingRageActive = false;
}

float AChampion_Tryndamere::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!HasAuthority() || !bUndyingRageActive || !StatComponent)
	{
		return Super::TakeDamage(
			DamageAmount,
			DamageEvent,
			EventInstigator,
			DamageCauser
		);
	}

	const float CurrentHP = StatComponent->GetCurrentHP();
	const float MaxAllowedDamage = FMath::Max(0.0f, CurrentHP - RMinimumHP);
	const float ClampedDamage = FMath::Min(DamageAmount, MaxAllowedDamage);

	if (ClampedDamage <= 0.0f)
	{
		StatComponent->SetHP(FMath::Max(CurrentHP, RMinimumHP));
		return 0.0f;
	}

	return Super::TakeDamage(
		ClampedDamage,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);
}

bool AChampion_Tryndamere::IsMoveInputBlocked() const
{
	return bMovementLocked || bESpinning;
}

void AChampion_Tryndamere::BeginMovementLock(float Duration)
{
	bMovementLocked = true;

	if (MoveComponent)
	{
		MoveComponent->StopMovement();
	}
	if (AttackComponent)
	{
		AttackComponent->CancelAttack();
		AttackComponent->CombatTarget = nullptr;
		AttackComponent->HitTarget = nullptr;
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	GetWorldTimerManager().ClearTimer(MovementLockTimerHandle);
	GetWorldTimerManager().SetTimer(
		MovementLockTimerHandle,
		this,
		&AChampion_Tryndamere::EndMovementLock,
		FMath::Max(Duration, 0.05f),
		false
	);
}

void AChampion_Tryndamere::EndMovementLock()
{
	bMovementLocked = false;

	if (!bESpinning && GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AChampion_Tryndamere::Multicast_PlayTryndamereSkillAnimation_Implementation(
	uint8 SkillIndex,
	float PlayRate,
	FRotator FacingRotation)
{
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	SetActorRotation(FacingRotation);

	if (UAnimMontage* Montage = GetTryndamereMontage(SkillIndex))
	{
		PlayAnimMontage(Montage, PlayRate);
	}
}

FVector AChampion_Tryndamere::ClampTargetLocation(
	FVector TargetLocation,
	float MaxRange) const
{
	const FVector Start = GetActorLocation();
	FVector Direction = TargetLocation - Start;
	Direction.Z = 0.0f;
	if (Direction.SizeSquared() > FMath::Square(MaxRange))
	{
		Direction = Direction.GetSafeNormal() * MaxRange;
	}

	FVector Result = Start + Direction;
	Result.Z = Start.Z;
	return Result;
}

float AChampion_Tryndamere::GetSkillValue(
	const TArray<float>& Values,
	int32 Index,
	float Fallback) const
{
	return Values.IsValidIndex(Index) ? Values[Index] : Fallback;
}

bool AChampion_Tryndamere::HasCastData(const FSkillData& SkillData) const
{
	return SkillData.ManaCost.IsValidIndex(0) &&
		SkillData.Cooldown.IsValidIndex(0);
}

UAnimMontage* AChampion_Tryndamere::GetTryndamereMontage(uint8 SkillIndex) const
{
	const TArray<UAnimMontage*>* Montages = nullptr;
	switch (SkillIndex)
	{
	case 0:
		Montages = &ChampionResource.QMontage;
		break;
	case 1:
		Montages = &ChampionResource.WMontage;
		break;
	case 2:
		Montages = &ChampionResource.EMontage;
		break;
	case 3:
		Montages = &ChampionResource.RMontage;
		break;
	default:
		break;
	}

	return Montages && Montages->IsValidIndex(0)
		? (*Montages)[0]
		: nullptr;
}

float AChampion_Tryndamere::GetAnimationDuration(
	uint8 SkillIndex,
	float Fallback) const
{
	if (UAnimMontage* Montage = GetTryndamereMontage(SkillIndex))
	{
		return FMath::Max(Montage->GetPlayLength(), 0.05f);
	}

	return Fallback;
}
