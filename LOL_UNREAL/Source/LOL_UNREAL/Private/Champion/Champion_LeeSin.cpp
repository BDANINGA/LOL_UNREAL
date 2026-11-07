#include "Champion/Champion_LeeSin.h"

#include "Animation/AnimMontage.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Minion/BaseMinion.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GamePlayTag/LOL_GamePlayTags.h"
#include "UObject/ConstructorHelpers.h"

AChampion_LeeSin::AChampion_LeeSin()
{
	ChampionName = TEXT("LeeSin");
	SetChampionData(ChampionName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectileMeshAsset(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (ProjectileMeshAsset.Succeeded())
	{
		SkillEffectPlane = ProjectileMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wave(
		TEXT("/Game/VFX/LeeSin/M_LeeSinSonicWave.M_LeeSinSonicWave"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mark(
		TEXT("/Game/VFX/LeeSin/M_LeeSinResonanceMark.M_LeeSinResonanceMark"));
	QWaveMaterial = Wave.Object;
	QMarkMaterial = Mark.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dash(
		TEXT("/Game/VFX/LeeSin/M_LeeSinWDash.M_LeeSinWDash"));
	WDashMaterial = Dash.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> KickImpact(
		TEXT("/Game/VFX/LeeSin/M_LeeSinRImpact.M_LeeSinRImpact"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> KickWake(
		TEXT("/Game/VFX/LeeSin/M_LeeSinRKnockback.M_LeeSinRKnockback"));
	KickImpactMaterial = KickImpact.Object;
	KickTrailMaterial = KickWake.Object;

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
	if (!SkillComponent || !StatComponent || bIsQDashing || bIsWDashing ||
		bQProjectileActive)
	{
		return;
	}

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
	const float TravelTime = Range / FMath::Max(QProjectileSpeed, 1.0f);

	QProjectileStart = Start;
	QProjectileEnd = End;
	QProjectileLastLocation = Start;
	QProjectileDirection = Direction;
	QProjectileElapsed = 0.0f;
	QProjectileTravelTime = TravelTime;
	bQProjectileActive = true;

	Multicast_SpawnQProjectile(Start, End, TravelTime);
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

	// Dash setup cancels the current attack montage. Play W after that cancellation.
	StartWDash(Destination);
	const UAnimMontage* DashMontage = GetLeeSinMontage(1, 0);
	const float PlayRate = DashMontage && WDashDuration > KINDA_SMALL_NUMBER && DashMontage->RateScale > 0.f
		? DashMontage->GetPlayLength() / (WDashDuration * DashMontage->RateScale)
		: 1.f;
	Multicast_PlayLeeSinSkillAnimation(1, 0, PlayRate, FacingRotation);

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

	BeginSkillMovementLock(EMovementLockDuration);
	Multicast_PlayLeeSinSkillAnimation(2, 0, 2.0f, GetActorRotation());

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

	Multicast_BeginKickVisual(Target, RKnockbackStart, Direction);
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

	if (IsValid(QMarkVisual) && IsValid(QMarkedTarget))
	{
		const auto* State = QMarkedTarget->FindComponentByClass<ULOL_StateComponent>();
		if (State && State->HasStatusTag(LOLTags::State_Dead))
		{
			RemoveQMarkVisual();
		}
	}

	if (HasAuthority() && bIsQDashing)
	{
		UpdateQDash(DeltaTime);
	}

	if (HasAuthority() && bQProjectileActive)
	{
		UpdateQProjectile(DeltaTime);
	}

	if (HasAuthority() && bIsWDashing)
	{
		UpdateWDash(DeltaTime);
	}

	if (WDashVisual)
	{
		constexpr float FadeSeconds = .16f;
		const bool bArrived = WVisualFadeStart >= 0.0;
		const float Opacity = bArrived
			? 1.f - (GetWorld()->GetTimeSeconds() - WVisualFadeStart) / FadeSeconds
			: 1.f;
		if (Opacity <= 0.f || (StateComponent && StateComponent->HasStatusTag(LOLTags::State_Dead)))
		{
			RemoveWDashVisual();
		}
		else
		{
			UpdateWDashVisual(bArrived ? WVisualArrival : GetActorLocation(), Opacity);
		}
	}

	if (HasAuthority() && bRKnockbackActive)
	{
		UpdateRKnockback(DeltaTime);
	}
}

bool AChampion_LeeSin::IsMoveInputBlocked() const
{
	return Super::IsMoveInputBlocked() || bSkillMovementLocked || bIsQDashing || bIsWDashing;
}

void AChampion_LeeSin::MarkQTarget(AActor* Target)
{
	QMarkedTarget = Target;
	bQMarkActive = IsValid(Target);
	OnRep_QMarkedTarget();
	ForceNetUpdate();

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
	OnRep_QMarkedTarget();
	ForceNetUpdate();
}

void AChampion_LeeSin::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AChampion_LeeSin, QMarkedTarget);
}

void AChampion_LeeSin::OnRep_QMarkedTarget()
{
	RemoveQMarkVisual();
	ACharacter* Target = Cast<ACharacter>(QMarkedTarget);
	if (GetNetMode() == NM_DedicatedServer || !IsValid(Target) || !QMarkMaterial) return;

	UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
	QMarkVisual = NewObject<UMaterialBillboardComponent>(Target);
	QMarkVisual->SetupAttachment(Capsule);
	QMarkVisual->SetAbsolute(false, true, true);
	QMarkVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QMarkVisual->SetGenerateOverlapEvents(false);
	QMarkVisual->SetCanEverAffectNavigation(false);
	QMarkVisual->SetCastShadow(false);
	auto* Material = UMaterialInstanceDynamic::Create(QMarkMaterial, QMarkVisual);
	Material->SetScalarParameterValue(TEXT("StartTime"), GetWorld()->GetTimeSeconds());
	QMarkVisual->AddElement(Material, nullptr, false, 48.f, 48.f, nullptr);
	QMarkVisual->RegisterComponent();
	QMarkVisual->SetWorldLocation(Capsule->GetComponentLocation() +
		FVector::UpVector * (Capsule->GetScaledCapsuleHalfHeight() + 58.f));
}

void AChampion_LeeSin::RemoveQMarkVisual()
{
	if (IsValid(QMarkVisual)) QMarkVisual->DestroyComponent();
	QMarkVisual = nullptr;
}

void AChampion_LeeSin::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(KickTrail)) KickTrail->DestroyComponent();
	RemoveWDashVisual();
	RemoveQMarkVisual();
	if (IsValid(QProjectileVisualActor)) QProjectileVisualActor->Destroy();
	Super::EndPlay(EndPlayReason);
}

void AChampion_LeeSin::UpdateQProjectile(float DeltaTime)
{
	if (!bQProjectileActive)
	{
		return;
	}

	QProjectileElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		QProjectileElapsed / FMath::Max(QProjectileTravelTime, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);
	const FVector CurrentLocation =
		FMath::Lerp(QProjectileStart, QProjectileEnd, Alpha);

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		QProjectileLastLocation,
		CurrentLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(QProjectileRadius),
		QueryParams
	);
	QProjectileLastLocation = CurrentLocation;

	if (bHit)
	{
		Hits.Sort([](const FHitResult& A, const FHitResult& B)
		{
			return A.Distance < B.Distance;
		});

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor) || HitActor == this) continue;

			const bool bCanHitWithQ =
				Cast<ABaseChampion>(HitActor) ||
				Cast<ABaseMinion>(HitActor) ||
				Cast<ABaseJungleMonster>(HitActor);
			if (!bCanHitWithQ) continue;
			if (!HitActor->FindComponentByClass<ULOL_StateComponent>()) continue;
			if (!IsEnemyActor(HitActor)) continue;

			ApplyQFirstHitDamage(HitActor);
			FinishQProjectile(true);
			return;
		}
	}

	if (Alpha >= 1.0f)
	{
		FinishQProjectile(true);
	}
}

void AChampion_LeeSin::FinishQProjectile(bool bDestroyVisual)
{
	bQProjectileActive = false;
	QProjectileElapsed = 0.0f;
	QProjectileTravelTime = 0.0f;
	QProjectileStart = FVector::ZeroVector;
	QProjectileEnd = FVector::ZeroVector;
	QProjectileLastLocation = FVector::ZeroVector;
	QProjectileDirection = FVector::ZeroVector;

	if (bDestroyVisual)
	{
		Multicast_DestroyQProjectile();
	}
}

void AChampion_LeeSin::ApplyQFirstHitDamage(AActor* Target)
{
	if (!HasAuthority() || !IsValid(Target) || !SkillComponent || !StatComponent)
	{
		return;
	}

	const FSkillData& QData = SkillComponent->GetQ_Data();
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
	MarkQTarget(Target);
}

void AChampion_LeeSin::StartQDash(AActor* Target)
{
	if (!IsValid(Target)) return;

	GetWorldTimerManager().ClearTimer(QMarkTimerHandle);
	GetWorldTimerManager().ClearTimer(SkillMovementLockTimerHandle);
	bSkillMovementLocked = false;
	ClearQMark();
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

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		QDashPreviousCollisionEnabled = Capsule->GetCollisionEnabled();
		QDashPreviousPawnResponse =
			Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
	if (UCapsuleComponent* TargetCapsule =
		Target->FindComponentByClass<UCapsuleComponent>())
	{
		TargetCapsule->IgnoreActorWhenMoving(this, true);
	}

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

	AActor* Target = QDashTarget;

	QDashElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		QDashElapsed / FMath::Max(QDashDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	FVector Direction = Target->GetActorLocation() - QDashStart;
	Direction.Z = 0.0f;
	float TargetRadius = 0.0f;
	if (UCapsuleComponent* TargetCapsule =
		Target->FindComponentByClass<UCapsuleComponent>())
	{
		TargetRadius = TargetCapsule->GetScaledCapsuleRadius();
	}
	const float TargetOffset =
		GetCapsuleComponent()->GetScaledCapsuleRadius() + TargetRadius;
	FVector LandingLocation =
		Target->GetActorLocation() -
		Direction.GetSafeNormal() * TargetOffset;
	LandingLocation.Z = QDashStart.Z;

	SetActorLocation(
		FMath::Lerp(QDashStart, LandingLocation, Alpha),
		false
	);

	if (Alpha >= 1.0f)
	{
		FinishQDash();
	}
}

void AChampion_LeeSin::FinishQDash()
{
	if (!bIsQDashing) return;

	AActor* Target = QDashTarget;
	bIsQDashing = false;
	QDashTarget = nullptr;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(
			ECC_Pawn,
			QDashPreviousPawnResponse
		);
		Capsule->SetCollisionEnabled(QDashPreviousCollisionEnabled);
	}

	if (IsValid(Target))
	{
		GetCapsuleComponent()->IgnoreActorWhenMoving(Target, false);
		if (UCapsuleComponent* TargetCapsule =
			Target->FindComponentByClass<UCapsuleComponent>())
		{
			TargetCapsule->IgnoreActorWhenMoving(this, false);
		}
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	ULOL_StatComponent* TargetStat =
		IsValid(Target)
			? Target->FindComponentByClass<ULOL_StatComponent>()
			: nullptr;
	if (!IsValid(Target) || !SkillComponent || !StatComponent || !TargetStat) return;

	const FSkillData& QData = SkillComponent->GetQ_Data();
	const float BaseDamage = GetSkillValue(QData.BaseDamage, 0, 55.0f);
	const float BaseSkillDamage =
		BaseDamage +
		StatComponent->GetStat().BonusAttackDamage * QBonusADRatio;

	const float MaxHP = FMath::Max(
		TargetStat->GetStat().MaxHP,
		1.0f
	);
	const float MissingHealthRatio = FMath::Clamp(
		1.0f - TargetStat->GetCurrentHP() / MaxHP,
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
	if (!WDashStart.Equals(WDashEnd, 1.f))
	{
		Multicast_BeginWDashVisual(WDashStart, WDashEnd);
	}

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

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		WDashPreviousCollisionEnabled = Capsule->GetCollisionEnabled();
		WDashPreviousPawnResponse =
			Capsule->GetCollisionResponseToChannel(ECC_Pawn);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		false
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
	Multicast_EndWDashVisual(GetActorLocation());

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(
			ECC_Pawn,
			WDashPreviousPawnResponse
		);
		Capsule->SetCollisionEnabled(WDashPreviousCollisionEnabled);
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

void AChampion_LeeSin::Multicast_BeginWDashVisual_Implementation(FVector Start, FVector Destination)
{
	if (GetNetMode() == NM_DedicatedServer || !SkillEffectPlane || !WDashMaterial) return;
	RemoveWDashVisual();
	WVisualStart = Start;
	WVisualDirection = (Destination - Start).GetSafeNormal2D();
	WVisualFadeStart = -1.0;

	WDashVisual = NewObject<UStaticMeshComponent>(this);
	WDashVisual->SetMobility(EComponentMobility::Movable);
	WDashVisual->SetStaticMesh(SkillEffectPlane);
	WDashVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WDashVisual->SetGenerateOverlapEvents(false);
	WDashVisual->SetCanEverAffectNavigation(false);
	WDashVisual->SetCastShadow(false);
	WDashVisual->bReceivesDecals = false;
	WDashVisual->SetWorldRotation(WVisualDirection.Rotation());
	WDashVisual->SetWorldScale3D(FVector(4.f, 2.f, 1.f));
	WDashMID = WDashVisual->CreateDynamicMaterialInstance(0, WDashMaterial);
	WDashMID->SetScalarParameterValue(TEXT("StartTime"), GetWorld()->GetTimeSeconds());
	UpdateWDashVisual(Start, 1.f);
	WDashVisual->RegisterComponent();
}

void AChampion_LeeSin::UpdateWDashVisual(FVector Location, float Opacity)
{
	// The crescent sits at the shoulders. Only reveal the ribbons over ground already crossed.
	const float Travel = FMath::Max(0.f, static_cast<float>(FVector::DotProduct(Location - WVisualStart, WVisualDirection)));
	WDashVisual->SetWorldLocation(Location - WVisualDirection * 100.f - FVector(0, 0, 25.f));
	WDashMID->SetScalarParameterValue(TEXT("Travel"), Travel);
	WDashMID->SetScalarParameterValue(TEXT("Fade"), Opacity);
}

void AChampion_LeeSin::Multicast_EndWDashVisual_Implementation(FVector Arrival)
{
	if (!WDashVisual) return;
	WVisualArrival = Arrival;
	WVisualFadeStart = GetWorld()->GetTimeSeconds();
	UpdateWDashVisual(Arrival, 1.f);
}

void AChampion_LeeSin::RemoveWDashVisual()
{
	if (WDashVisual) WDashVisual->DestroyComponent();
	WDashVisual = nullptr;
	WDashMID = nullptr;
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

	UCapsuleComponent* Capsule = RKnockbackTarget->GetCapsuleComponent();
	const FVector CurrentLocation = Capsule->GetComponentLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeSinKickObstacles), false, this);
	QueryParams.AddIgnoredActor(RKnockbackTarget);

	// Buildings use Pawn and some barriers use WorldDynamic. Respect the victim's
	// walking collision, while allowing other champions to be struck along the arc.
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, CurrentLocation, DesiredLocation,
		Capsule->GetComponentQuat(), FCollisionObjectQueryParams::AllObjects,
		Capsule->GetCollisionShape(), QueryParams);

	const FHitResult* Obstacle = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* Other = Hit.GetComponent();
		if (!Other || Cast<ACharacter>(Hit.GetActor()) ||
			Capsule->GetCollisionResponseToComponent(Other) != ECR_Block)
		{
			continue;
		}
		if (Hit.bStartPenetrating &&
			FVector::DotProduct(DesiredLocation - CurrentLocation, Hit.Normal) > 0.f)
		{
			continue;
		}
		if (!Obstacle || Hit.Time < Obstacle->Time) Obstacle = &Hit;
	}

	const FVector SafeLocation = Obstacle ? FVector(Obstacle->Location) : DesiredLocation;
	RKnockbackTarget->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (Obstacle || Alpha >= 1.0f)
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
		Multicast_KickCollisionVisual(HitChampion, HitChampion->GetActorLocation());
		HitChampion->StartKnockbackWithWallCheck(
			FVector(0.0f, 0.0f, RCollisionAirborneVelocity),
			RCollisionAirborneDuration,
			0.0f
		);
	}
}

void AChampion_LeeSin::FinishRKnockbackCollision()
{
	if (bRKnockbackActive) Multicast_EndKickVisual();
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

namespace
{
void RegisterKickLayer(UPrimitiveComponent* Layer, float Lifetime)
{
	Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Layer->SetGenerateOverlapEvents(false);
	Layer->SetCanEverAffectNavigation(false);
	Layer->SetCastShadow(false);
	Layer->bReceivesDecals = false;
	Layer->RegisterComponent();
	FTimerHandle Handle;
	Layer->GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(
		Layer, [Layer]() { Layer->DestroyComponent(); }), Lifetime, false);
}
}

void AChampion_LeeSin::ShowKickImpact(ABaseChampion* Target, FVector Location, float Size)
{
	if (!IsValid(Target) || !KickImpactMaterial || GetNetMode() == NM_DedicatedServer) return;
	auto* Flash = NewObject<UMaterialBillboardComponent>(Target);
	auto* Material = UMaterialInstanceDynamic::Create(KickImpactMaterial, Flash);
	Material->SetScalarParameterValue(TEXT("StartTime"), GetWorld()->GetTimeSeconds());
	Flash->AddElement(Material, nullptr, false, Size, Size, nullptr);
	Flash->SetWorldLocation(Location);
	RegisterKickLayer(Flash, .33f);
}

void AChampion_LeeSin::Multicast_BeginKickVisual_Implementation(
	ABaseChampion* Target, FVector Contact, FVector_NetQuantizeNormal Direction)
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(Target)) return;
	ShowKickImpact(Target, Contact - Direction * Target->GetCapsuleComponent()->GetScaledCapsuleRadius(), 132.f);
	ReleaseKickTrail();
	if (!KickTrailMaterial || !SkillEffectPlane) return;

	KickTrail = NewObject<UStaticMeshComponent>(Target);
	KickTrail->SetupAttachment(Target->GetRootComponent());
	KickTrail->SetAbsolute(false, true, true);
	KickTrail->SetWorldRotation(Direction.Rotation());
	KickTrail->SetWorldScale3D(FVector(5.9f, 2.1f, 1.f));
	KickTrail->SetStaticMesh(SkillEffectPlane);
	auto* Material = KickTrail->CreateDynamicMaterialInstance(0, KickTrailMaterial);
	const float Now = GetWorld()->GetTimeSeconds();
	Material->SetScalarParameterValue(TEXT("StartTime"), Now);
	Material->SetScalarParameterValue(TEXT("EndTime"), Now + RKnockbackDuration);
	RegisterKickLayer(KickTrail, RKnockbackDuration + .15f);
}

void AChampion_LeeSin::Multicast_KickCollisionVisual_Implementation(ABaseChampion* Target, FVector Contact)
{
	ShowKickImpact(Target, Contact, 88.f);
}

void AChampion_LeeSin::Multicast_EndKickVisual_Implementation()
{
	ReleaseKickTrail();
}

void AChampion_LeeSin::ReleaseKickTrail()
{
	if (IsValid(KickTrail))
	{
		KickTrail->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (auto* Material = Cast<UMaterialInstanceDynamic>(KickTrail->GetMaterial(0)))
		{
			Material->SetScalarParameterValue(TEXT("EndTime"), GetWorld()->GetTimeSeconds());
		}
	}
	KickTrail = nullptr;
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
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer || !QWaveMaterial) return;

	if (IsValid(QProjectileVisualActor))
	{
		QProjectileVisualActor->Destroy();
		QProjectileVisualActor = nullptr;
	}

	UStaticMesh* ProjectileMeshAsset = SkillEffectPlane.Get();
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
	QProjectileVisualActor = ProjectileActor;

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
	ProjectileMesh->SetMaterial(0, QWaveMaterial);
	ProjectileMesh->SetCanEverAffectNavigation(false);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCastShadow(false);
	ProjectileMesh->RegisterComponent();

	// The leading pulse is at UV x = .73. Keep it on the server's sweep centre.
	const float Size = QProjectileVisualRadius * QProjectileVisualScale;
	const FQuat Orientation = Direction.Rotation().Quaternion() * QProjectileRotationOffset.Quaternion();
	ProjectileActor->SetActorLocationAndRotation(
		StartLocation - Orientation.GetForwardVector() * (.46f * 2.4f * Size),
		Orientation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
	ProjectileMesh->SetWorldScale3D(FVector(4.8f * Size / 100.f, 2.25f * Size / 100.f, 1.f));
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
		FMath::Max(TravelTime, 0.05f)
	);
}

void AChampion_LeeSin::Multicast_DestroyQProjectile_Implementation()
{
	if (IsValid(QProjectileVisualActor))
	{
		QProjectileVisualActor->Destroy();
		QProjectileVisualActor = nullptr;
	}
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
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("LeeSin montage missing. SkillIndex=%d MontageIndex=%d"),
			SkillIndex,
			MontageIndex
		);
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
