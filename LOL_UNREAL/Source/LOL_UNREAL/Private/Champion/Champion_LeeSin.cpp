#include "Champion/Champion_LeeSin.h"

#include "Animation/AnimMontage.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AChampion_LeeSin::AChampion_LeeSin()
{
	ChampionName = TEXT("LeeSin");
	SetChampionData(ChampionName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectileMeshAsset(
		TEXT("/Game/Level/leesin/leesin_tex/leesin_q.leesin_q"));
	if (ProjectileMeshAsset.Succeeded())
	{
		DefaultQProjectileMesh = ProjectileMeshAsset.Object;
	}

	GetMesh()->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));
}

void AChampion_LeeSin::Skill_Q()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FVector TargetLocation =
		GetActorLocation() + GetActorForwardVector() * 1100.0f;
	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		TargetLocation = Hit.ImpactPoint;
	}

	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.IsNearlyZero())
	{
		SetActorRotation(Direction.Rotation());
	}

	Server_Skill_Q(TargetLocation);
}

void AChampion_LeeSin::Skill_W()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		FVector Direction = Hit.ImpactPoint - GetActorLocation();
		Direction.Z = 0.0f;
		if (!Direction.IsNearlyZero())
		{
			SetActorRotation(Direction.Rotation());
		}

		Server_Skill_W(Hit.ImpactPoint);
	}
}

void AChampion_LeeSin::Skill_E()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
	Server_Skill_E();
}

void AChampion_LeeSin::Skill_R()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FHitResult Hit;
	if (!PlayerController->GetHitResultUnderCursor(ECC_Pawn, false, Hit)) return;

	ABaseChampion* Target = Cast<ABaseChampion>(Hit.GetActor());
	if (IsValid(Target) && Target != this)
	{
		Server_Skill_R(Target);
	}
}

bool AChampion_LeeSin::Server_Skill_Q_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_LeeSin::Server_Skill_Q_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bIsQDashing || bIsWDashing) return;

	if (bQMarkActive && IsValid(QMarkedTarget))
	{
		const float RecastRange = GetSkillValue(
			SkillComponent->GetQ_Data().Range,
			0,
			1100.0f
		) + 150.0f;

		if (GetDistanceTo(QMarkedTarget) <= RecastRange)
		{
			StartQDash(QMarkedTarget);
		}
		return;
	}

	const FSkillData& QData = SkillComponent->GetQ_Data();
	if (!HasCastData(QData)) return;
	if (!SkillComponent->TryCastSkill("Q", 1)) return;

	const float Range = GetSkillValue(QData.Range, 0, 1100.0f);

	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;
	Direction.Normalize();

	const FRotator FacingRotation = Direction.Rotation();
	BeginSkillMovementLock(
		GetSkillMovementLockDuration(0, 0, 0.55f)
	);
	Multicast_PlayLeeSinSkillAnimation(0, 0, 1.0f, FacingRotation);

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
	const FVector End = Start + Direction * Range;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(QProjectileRadius),
		QueryParams
	);

	const float ProjectileDistance = bHit
		? FMath::Clamp(Hit.Distance, 0.0f, Range)
		: Range;
	const FVector ProjectileEnd = Start + Direction * ProjectileDistance;
	const float TravelTime =
		ProjectileDistance / FMath::Max(QProjectileSpeed, 1.0f);
	Multicast_SpawnQProjectile(Start, ProjectileEnd, TravelTime);

	AActor* Target = bHit ? Hit.GetActor() : nullptr;
	if (!IsValid(Target) || Target == this ||
		!Target->FindComponentByClass<ULOL_StateComponent>() || !IsEnemyActor(Target))
	{
		return;
	}

	const float BaseDamage = GetSkillValue(QData.BaseDamage, 0, 55.0f);
	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().BonusAttackDamage * QBonusADRatio;

	UGameplayStatics::ApplyDamage(
		Target,
		SkillDamage,
		GetController(),
		this,
		ULOL_DamagePhysical::StaticClass()
	);
	if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
	{
		MarkQTarget(TargetChampion);
	}
}

bool AChampion_LeeSin::Server_Skill_W_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_LeeSin::Server_Skill_W_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bIsQDashing || bIsWDashing) return;
	const FSkillData& WData = SkillComponent->GetW_Data();
	if (!HasCastData(WData)) return;
	if (!SkillComponent->TryCastSkill("W", 1)) return;

	const float Range = GetSkillValue(WData.Range, 0, 700.0f);
	const FVector Destination = ClampTargetLocation(TargetLocation, Range);

	FVector Direction = Destination - GetActorLocation();
	Direction.Z = 0.0f;
	const FRotator FacingRotation = Direction.IsNearlyZero()
		? GetActorRotation()
		: Direction.Rotation();

	Multicast_PlayLeeSinSkillAnimation(1, 0, 1.0f, FacingRotation);
	StartWDash(Destination);

	const float BaseShield = GetSkillValue(WData.BaseDamage, 0, 60.0f);
	const float ShieldAmount =
		BaseShield +
		StatComponent->GetStat().AbilityPower * WAbilityPowerRatio;
	WShieldRemaining = FMath::Max(WShieldRemaining, ShieldAmount);

	const float Duration = GetSkillValue(
		WData.Duration,
		0,
		WShieldDuration
	);
	GetWorldTimerManager().ClearTimer(WShieldTimerHandle);
	GetWorldTimerManager().SetTimer(
		WShieldTimerHandle,
		this,
		&AChampion_LeeSin::EndWShield,
		FMath::Max(Duration, 0.1f),
		false
	);
}

bool AChampion_LeeSin::Server_Skill_E_Validate()
{
	return true;
}

void AChampion_LeeSin::Server_Skill_E_Implementation()
{
	if (!SkillComponent || !StatComponent || bIsQDashing || bIsWDashing) return;
	const FSkillData& EData = SkillComponent->GetE_Data();
	if (!HasCastData(EData)) return;
	if (!SkillComponent->TryCastSkill("E", 1)) return;

	BeginSkillMovementLock(
		GetSkillMovementLockDuration(2, 0, 0.7f)
	);
	Multicast_PlayLeeSinSkillAnimation(2, 0, 1.0f, GetActorRotation());

	const float Radius = GetSkillValue(EData.Range, 0, 350.0f);
	const float BaseDamage = GetSkillValue(EData.BaseDamage, 0, 35.0f);
	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().BonusAttackDamage * EBonusADRatio;

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

	TSet<TWeakObjectPtr<AActor>> DamagedTargets;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!IsValid(Target) || Target == this || DamagedTargets.Contains(Target)) continue;
		if (!Target->FindComponentByClass<ULOL_StateComponent>() || !IsEnemyActor(Target)) continue;
		DamagedTargets.Add(Target);

		UGameplayStatics::ApplyDamage(
			Target,
			SkillDamage,
			GetController(),
			this,
			ULOL_DamageMagic::StaticClass()
		);

		ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target);
		if (!TargetChampion) continue;

		UCharacterMovementComponent* TargetMovement =
			TargetChampion->GetCharacterMovement();
		if (!TargetMovement) continue;

		const TWeakObjectPtr<ABaseChampion> TargetKey(TargetChampion);
		if (FTimerHandle* ExistingTimer = ESlowTimerHandles.Find(TargetKey))
		{
			GetWorldTimerManager().ClearTimer(*ExistingTimer);
		}

		const float BaseMoveSpeed =
			TargetChampion->StatComponent &&
			TargetChampion->StatComponent->GetStat().MoveSpeed > 0.0f
				? TargetChampion->StatComponent->GetStat().MoveSpeed
				: TargetMovement->MaxWalkSpeed;
		TargetMovement->MaxWalkSpeed =
			BaseMoveSpeed * (1.0f - FMath::Clamp(ESlowRatio, 0.0f, 0.9f));

		FTimerHandle& SlowTimer = ESlowTimerHandles.FindOrAdd(TargetKey);
		FTimerDelegate RestoreDelegate;
		RestoreDelegate.BindUObject(
			this,
			&AChampion_LeeSin::RestoreTargetMoveSpeed,
			TargetKey
		);
		GetWorldTimerManager().SetTimer(
			SlowTimer,
			RestoreDelegate,
			ESlowDuration,
			false
		);
	}
}

bool AChampion_LeeSin::Server_Skill_R_Validate(ABaseChampion* Target)
{
	return true;
}

void AChampion_LeeSin::Server_Skill_R_Implementation(ABaseChampion* Target)
{
	if (!SkillComponent || !StatComponent || bIsQDashing || bIsWDashing || !IsValid(Target) || Target == this) return;

	const FSkillData& RData = SkillComponent->GetR_Data();
	if (!HasCastData(RData)) return;
	const float Range = GetSkillValue(RData.Range, 0, 375.0f);
	if (GetDistanceTo(Target) > Range + 50.0f) return;
	if (!SkillComponent->TryCastSkill("R", 1)) return;

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;
	Direction.Normalize();

	BeginSkillMovementLock(
		GetSkillMovementLockDuration(3, 0, 0.8f)
	);
	Multicast_PlayLeeSinSkillAnimation(3, 0, 1.0f, Direction.Rotation());

	const float BaseDamage = GetSkillValue(RData.BaseDamage, 0, 175.0f);
	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().BonusAttackDamage * RBonusADRatio;

	UGameplayStatics::ApplyDamage(
		Target,
		SkillDamage,
		GetController(),
		this,
		ULOL_DamagePhysical::StaticClass()
	);

	GetWorldTimerManager().ClearTimer(RCollisionCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(RCollisionEndTimerHandle);
	RKnockbackTarget = Target;
	RKnockbackStart = Target->GetActorLocation();
	RKnockbackEnd = RKnockbackStart + Direction * RKnockbackDistance;
	RKnockbackEnd.Z = RKnockbackStart.Z;
	RKnockbackLastLocation = RKnockbackStart;
	RKnockbackElapsed = 0.0f;
	bRKnockbackActive = true;
	RAirborneTargets.Empty();

	Target->SetIsKnockedBack(true);
	if (UCharacterMovementComponent* TargetMovement =
		Target->GetCharacterMovement())
	{
		TargetMovement->StopMovementImmediately();
		TargetMovement->DisableMovement();
	}

	GetWorldTimerManager().SetTimer(
		RCollisionCheckTimerHandle,
		this,
		&AChampion_LeeSin::CheckRKnockbackCollision,
		0.02f,
		true
	);
	GetWorldTimerManager().SetTimer(
		RCollisionEndTimerHandle,
		this,
		&AChampion_LeeSin::FinishRKnockbackCollision,
		RKnockbackDuration + 0.1f,
		false
	);
}

void AChampion_LeeSin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bIsQDashing)
	{
		UpdateQDash(DeltaTime);
	}

	if (HasAuthority() && bIsWDashing)
	{
		UpdateWDash(DeltaTime);
	}

	if (HasAuthority() && bRKnockbackActive)
	{
		UpdateRKnockback(DeltaTime);
	}
}

bool AChampion_LeeSin::IsMoveInputBlocked() const
{
	return bSkillMovementLocked || bIsQDashing || bIsWDashing;
}

void AChampion_LeeSin::MarkQTarget(ABaseChampion* Target)
{
	QMarkedTarget = Target;
	bQMarkActive = IsValid(Target);

	GetWorldTimerManager().ClearTimer(QMarkTimerHandle);
	if (bQMarkActive)
	{
		GetWorldTimerManager().SetTimer(
			QMarkTimerHandle,
			this,
			&AChampion_LeeSin::ClearQMark,
			QMarkDuration,
			false
		);
	}
}

void AChampion_LeeSin::ClearQMark()
{
	bQMarkActive = false;
	QMarkedTarget = nullptr;
}

void AChampion_LeeSin::StartQDash(ABaseChampion* Target)
{
	if (!IsValid(Target)) return;

	GetWorldTimerManager().ClearTimer(QMarkTimerHandle);
	GetWorldTimerManager().ClearTimer(SkillMovementLockTimerHandle);
	bSkillMovementLocked = false;
	bQMarkActive = false;
	QDashTarget = Target;
	QDashStart = GetActorLocation();
	QDashElapsed = 0.0f;
	bIsQDashing = true;

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

	GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
	Target->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	Multicast_PlayLeeSinSkillAnimation(
		0,
		1,
		1.0f,
		Direction.IsNearlyZero() ? GetActorRotation() : Direction.Rotation()
	);
}

void AChampion_LeeSin::BeginSkillMovementLock(float Duration)
{
	bSkillMovementLocked = true;

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

	GetWorldTimerManager().ClearTimer(SkillMovementLockTimerHandle);
	GetWorldTimerManager().SetTimer(
		SkillMovementLockTimerHandle,
		this,
		&AChampion_LeeSin::EndSkillMovementLock,
		FMath::Max(Duration, 0.05f),
		false
	);
}

void AChampion_LeeSin::EndSkillMovementLock()
{
	bSkillMovementLocked = false;

	if (!bIsQDashing && !bIsWDashing && GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

float AChampion_LeeSin::GetSkillMovementLockDuration(
	uint8 SkillIndex,
	int32 MontageIndex,
	float Fallback) const
{
	if (UAnimMontage* Montage =
		GetLeeSinMontage(SkillIndex, MontageIndex))
	{
		return FMath::Max(Montage->GetPlayLength(), 0.05f);
	}

	return Fallback;
}

void AChampion_LeeSin::UpdateQDash(float DeltaTime)
{
	if (!IsValid(QDashTarget))
	{
		FinishQDash();
		return;
	}

	QDashElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		QDashElapsed / FMath::Max(QDashDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	FVector Direction = QDashTarget->GetActorLocation() - QDashStart;
	Direction.Z = 0.0f;
	const float TargetOffset =
		GetCapsuleComponent()->GetScaledCapsuleRadius() +
		QDashTarget->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector LandingLocation =
		QDashTarget->GetActorLocation() -
		Direction.GetSafeNormal() * TargetOffset;
	LandingLocation.Z = QDashStart.Z;

	SetActorLocation(
		FMath::Lerp(QDashStart, LandingLocation, Alpha),
		true
	);

	if (Alpha >= 1.0f)
	{
		FinishQDash();
	}
}

void AChampion_LeeSin::FinishQDash()
{
	if (!bIsQDashing) return;

	ABaseChampion* Target = QDashTarget;
	bIsQDashing = false;
	QDashTarget = nullptr;
	QMarkedTarget = nullptr;

	if (IsValid(Target))
	{
		GetCapsuleComponent()->IgnoreActorWhenMoving(Target, false);
		Target->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (!IsValid(Target) || !SkillComponent || !StatComponent || !Target->StatComponent) return;

	const FSkillData& QData = SkillComponent->GetQ_Data();
	const float BaseDamage = GetSkillValue(QData.BaseDamage, 0, 55.0f);
	const float BaseSkillDamage =
		BaseDamage +
		StatComponent->GetStat().BonusAttackDamage * QBonusADRatio;

	const float MaxHP = FMath::Max(
		Target->StatComponent->GetStat().MaxHP,
		1.0f
	);
	const float MissingHealthRatio = FMath::Clamp(
		1.0f - Target->StatComponent->GetCurrentHP() / MaxHP,
		0.0f,
		1.0f
	);
	const float SkillDamage = BaseSkillDamage * (
		1.0f +
		MissingHealthRatio * QMissingHealthMaxBonusRatio
	);

	UGameplayStatics::ApplyDamage(
		Target,
		SkillDamage,
		GetController(),
		this,
		ULOL_DamagePhysical::StaticClass()
	);
}

void AChampion_LeeSin::StartWDash(FVector Destination)
{
	WDashStart = GetActorLocation();
	WDashEnd = Destination;
	WDashEnd.Z = WDashStart.Z;
	WDashElapsed = 0.0f;
	bIsWDashing = true;

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

	TArray<AActor*> Champions;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ABaseChampion::StaticClass(),
		Champions
	);
	for (AActor* Actor : Champions)
	{
		ABaseChampion* Champion = Cast<ABaseChampion>(Actor);
		if (!IsValid(Champion) || Champion == this) continue;

		GetCapsuleComponent()->IgnoreActorWhenMoving(Champion, true);
		Champion->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
	}
}

void AChampion_LeeSin::UpdateWDash(float DeltaTime)
{
	WDashElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		WDashElapsed / FMath::Max(WDashDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	SetActorLocation(
		FMath::Lerp(WDashStart, WDashEnd, Alpha),
		true
	);

	if (Alpha >= 1.0f)
	{
		FinishWDash();
	}
}

void AChampion_LeeSin::FinishWDash()
{
	if (!bIsWDashing) return;

	bIsWDashing = false;

	TArray<AActor*> Champions;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		ABaseChampion::StaticClass(),
		Champions
	);
	for (AActor* Actor : Champions)
	{
		ABaseChampion* Champion = Cast<ABaseChampion>(Actor);
		if (!IsValid(Champion) || Champion == this) continue;

		GetCapsuleComponent()->IgnoreActorWhenMoving(Champion, false);
		Champion->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AChampion_LeeSin::EndWShield()
{
	WShieldRemaining = 0.0f;
}

void AChampion_LeeSin::RestoreTargetMoveSpeed(
	TWeakObjectPtr<ABaseChampion> Target)
{
	if (Target.IsValid() && Target->GetCharacterMovement())
	{
		const float BaseMoveSpeed =
			Target->StatComponent &&
			Target->StatComponent->GetStat().MoveSpeed > 0.0f
				? Target->StatComponent->GetStat().MoveSpeed
				: 330.0f;
		Target->GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	}

	ESlowTimerHandles.Remove(Target);
}

void AChampion_LeeSin::UpdateRKnockback(float DeltaTime)
{
	if (!IsValid(RKnockbackTarget))
	{
		FinishRKnockbackCollision();
		return;
	}

	RKnockbackElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		RKnockbackElapsed /
			FMath::Max(RKnockbackDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	FVector DesiredLocation =
		FMath::Lerp(RKnockbackStart, RKnockbackEnd, Alpha);
	DesiredLocation.Z +=
		FMath::Sin(Alpha * PI) * RKnockbackArcHeight;

	const float Radius =
		RKnockbackTarget->GetCapsuleComponent()
			->GetScaledCapsuleRadius();
	const float HalfHeight =
		RKnockbackTarget->GetCapsuleComponent()
			->GetScaledCapsuleHalfHeight();

	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(RKnockbackTarget);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const bool bWorldHit = GetWorld()->SweepSingleByObjectType(
		WallHit,
		RKnockbackTarget->GetActorLocation(),
		DesiredLocation,
		RKnockbackTarget->GetActorQuat(),
		ObjectParams,
		FCollisionShape::MakeCapsule(
			Radius * 0.9f,
			HalfHeight * 0.9f
		),
		QueryParams
	);
	const bool bHitWall =
		bWorldHit &&
		(WallHit.bStartPenetrating ||
			FMath::Abs(WallHit.ImpactNormal.Z) < 0.5f);

	if (bHitWall)
	{
		RKnockbackTarget->SetActorLocation(
			WallHit.Location,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
		CheckRKnockbackCollision();
		FinishRKnockbackCollision();
		return;
	}

	RKnockbackTarget->SetActorLocation(
		DesiredLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (Alpha >= 1.0f)
	{
		CheckRKnockbackCollision();
		FinishRKnockbackCollision();
	}
}

void AChampion_LeeSin::CheckRKnockbackCollision()
{
	if (!HasAuthority() || !IsValid(RKnockbackTarget))
	{
		FinishRKnockbackCollision();
		return;
	}

	const FVector CurrentLocation = RKnockbackTarget->GetActorLocation();
	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(RKnockbackTarget);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		RKnockbackLastLocation,
		CurrentLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(RCollisionRadius),
		QueryParams
	);
	RKnockbackLastLocation = CurrentLocation;

	if (!bHit) return;

	for (const FHitResult& Hit : Hits)
	{
		ABaseChampion* HitChampion =
			Cast<ABaseChampion>(Hit.GetActor());
		if (!IsValid(HitChampion) ||
			HitChampion == this ||
			HitChampion == RKnockbackTarget ||
			RAirborneTargets.Contains(HitChampion))
		{
			continue;
		}

		RAirborneTargets.Add(HitChampion);
		HitChampion->StartKnockbackWithWallCheck(
			FVector(0.0f, 0.0f, RCollisionAirborneVelocity),
			RCollisionAirborneDuration,
			0.0f
		);
	}
}

void AChampion_LeeSin::FinishRKnockbackCollision()
{
	if (IsValid(RKnockbackTarget))
	{
		RKnockbackTarget->SetIsKnockedBack(false);
		if (UCharacterMovementComponent* TargetMovement =
			RKnockbackTarget->GetCharacterMovement())
		{
			TargetMovement->SetMovementMode(MOVE_Walking);
		}
	}

	GetWorldTimerManager().ClearTimer(RCollisionCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(RCollisionEndTimerHandle);
	bRKnockbackActive = false;
	RKnockbackTarget = nullptr;
	RAirborneTargets.Empty();
}

float AChampion_LeeSin::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (HasAuthority() && WShieldRemaining > 0.0f && DamageAmount > 0.0f)
	{
		const float AbsorbedDamage =
			FMath::Min(WShieldRemaining, DamageAmount);
		WShieldRemaining -= AbsorbedDamage;
		DamageAmount -= AbsorbedDamage;

		if (DamageAmount <= 0.0f)
		{
			return 0.0f;
		}
	}

	return Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);
}

void AChampion_LeeSin::Multicast_SpawnQProjectile_Implementation(
	FVector StartLocation,
	FVector EndLocation,
	float TravelTime)
{
	if (!GetWorld()) return;

	UStaticMesh* ProjectileMeshAsset = DefaultQProjectileMesh.Get();
	if (!ProjectileMeshAsset) return;

	const FVector Direction =
		(EndLocation - StartLocation).GetSafeNormal();
	if (Direction.IsNearlyZero()) return;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* ProjectileActor = GetWorld()->SpawnActor<AActor>(
		AActor::StaticClass(),
		StartLocation,
		Direction.Rotation(),
		SpawnParameters
	);
	if (!ProjectileActor) return;

	UStaticMeshComponent* ProjectileMesh =
		NewObject<UStaticMeshComponent>(
			ProjectileActor,
			TEXT("LeeSinQProjectileMesh")
		);
	if (!ProjectileMesh)
	{
		ProjectileActor->Destroy();
		return;
	}

	ProjectileActor->AddInstanceComponent(ProjectileMesh);
	ProjectileActor->SetRootComponent(ProjectileMesh);
	ProjectileMesh->SetStaticMesh(ProjectileMeshAsset);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCastShadow(false);
	ProjectileMesh->RegisterComponent();

	ProjectileActor->SetActorLocationAndRotation(
		StartLocation,
		Direction.Rotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
	ProjectileMesh->SetRelativeRotation(QProjectileRotationOffset);

	const float MeshRadius =
		ProjectileMeshAsset->GetBounds().SphereRadius;
	const float ProjectileScale = MeshRadius > KINDA_SMALL_NUMBER
		? QProjectileVisualRadius / MeshRadius
		: 1.0f;
	ProjectileMesh->SetWorldScale3D(FVector(ProjectileScale * QProjectileVisualScale));
	ProjectileMesh->SetVisibility(true, true);
	ProjectileMesh->SetHiddenInGame(false, true);

	UProjectileMovementComponent* ProjectileMovement =
		NewObject<UProjectileMovementComponent>(
			ProjectileActor,
			TEXT("LeeSinQProjectileMovement")
		);
	if (!ProjectileMovement)
	{
		ProjectileActor->Destroy();
		return;
	}

	const float VisualSpeed =
		FVector::Distance(StartLocation, EndLocation) /
		FMath::Max(TravelTime, KINDA_SMALL_NUMBER);

	ProjectileActor->AddInstanceComponent(ProjectileMovement);
	ProjectileMovement->SetUpdatedComponent(ProjectileMesh);
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->InitialSpeed = VisualSpeed;
	ProjectileMovement->MaxSpeed = VisualSpeed;
	ProjectileMovement->Velocity = Direction * VisualSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->RegisterComponent();
	ProjectileMovement->Activate(true);

	ProjectileActor->SetLifeSpan(
		FMath::Max(TravelTime + 0.1f, 0.2f)
	);
}

void AChampion_LeeSin::Multicast_PlayLeeSinSkillAnimation_Implementation(
	uint8 SkillIndex,
	int32 MontageIndex,
	float PlayRate,
	FRotator FacingRotation)
{
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	SetActorRotation(FacingRotation);

	if (UAnimMontage* Montage =
		GetLeeSinMontage(SkillIndex, MontageIndex))
	{
		PlayAnimMontage(Montage, PlayRate);
	}
}

FVector AChampion_LeeSin::ClampTargetLocation(
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

float AChampion_LeeSin::GetSkillValue(
	const TArray<float>& Values,
	int32 Index,
	float Fallback) const
{
	return Values.IsValidIndex(Index) ? Values[Index] : Fallback;
}

bool AChampion_LeeSin::HasCastData(const FSkillData& SkillData) const
{
	return SkillData.ManaCost.IsValidIndex(0) &&
		SkillData.Cooldown.IsValidIndex(0);
}

UAnimMontage* AChampion_LeeSin::GetLeeSinMontage(
	uint8 SkillIndex,
	int32 MontageIndex) const
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

	return Montages && Montages->IsValidIndex(MontageIndex)
		? (*Montages)[MontageIndex]
		: nullptr;
}
