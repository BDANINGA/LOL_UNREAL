#include "Champion/Champion_Gragas.h"

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
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AChampion_Gragas::AChampion_Gragas()
{
	ChampionName = TEXT("Gragas");
	SetChampionData(ChampionName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> QMeshAsset(
		TEXT("/Game/Level/gragas/tex_gragas/throw_q.throw_q"));
	if (QMeshAsset.Succeeded())
	{
		QProjectileMesh = QMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RMeshAsset(
		TEXT("/Game/Level/gragas/tex_gragas/throw_r.throw_r"));
	if (RMeshAsset.Succeeded())
	{
		RProjectileMesh = RMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FloatEffectAsset(
		TEXT("/Game/Level/gragas/tex_gragas/ns_gragas_float.ns_gragas_float"));
	if (FloatEffectAsset.Succeeded())
	{
		FloatEffectSystem = FloatEffectAsset.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Gragas float Niagara effect failed to load."));
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> QFloatEffectAsset(
		TEXT("/Game/Level/gragas/tex_gragas/ns_gragas_float_q.ns_gragas_float_q"));
	if (QFloatEffectAsset.Succeeded())
	{
		QFloatEffectSystem = QFloatEffectAsset.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Gragas Q float Niagara effect failed to load."));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Game/Level/gragas/mat_gragas/m_gragas_q.m_gragas_q"));
	if (MaterialAsset.Succeeded())
	{
		ProjectileMaterial = MaterialAsset.Object;
	}

}

void AChampion_Gragas::Skill_Q()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FVector TargetLocation =
		GetActorLocation() + GetActorForwardVector() * 850.0f;
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

void AChampion_Gragas::Skill_W()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;
	Server_Skill_W();
}

void AChampion_Gragas::Skill_E()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack || bEDashing) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FVector TargetLocation =
		GetActorLocation() + GetActorForwardVector() * 600.0f;
	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		TargetLocation = Hit.ImpactPoint;
	}

	Server_Skill_E(TargetLocation);
}

void AChampion_Gragas::Skill_R()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FVector TargetLocation =
		GetActorLocation() + GetActorForwardVector() * 1000.0f;
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

	Server_Skill_R(TargetLocation);
}

bool AChampion_Gragas::Server_Skill_Q_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_Gragas::Server_Skill_Q_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bEDashing) return;

	if (bQActive)
	{
		ExplodeQ();
		return;
	}

	const FSkillData& QData = SkillComponent->GetQ_Data();
	if (!HasCastData(QData) || !SkillComponent->TryCastSkill("Q", 1)) return;

	const float Range = GetSkillValue(QData.Range, 0, 850.0f);
	QExplosionLocation = ClampTargetLocation(TargetLocation, Range);

	FVector Direction = QExplosionLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;

	BeginMovementLock(GetAnimationDuration(0, 0.55f));
	Multicast_PlayGragasSkillAnimation(0, 1.0f, Direction.Rotation());

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	QExplosionLocation.Z = Start.Z;
	QTravelTime =
		FVector::Dist2D(Start, QExplosionLocation) /
		FMath::Max(QProjectileSpeed, 1.0f);
	QCastStartTime = GetWorld()->GetTimeSeconds();
	bQActive = true;

	Multicast_SpawnGragasProjectile(
		0,
		Start,
		QExplosionLocation,
		QTravelTime,
		QTravelTime + QFuseDuration + 0.25f
	);

	GetWorldTimerManager().ClearTimer(QExplosionTimerHandle);
	GetWorldTimerManager().SetTimer(
		QExplosionTimerHandle,
		this,
		&AChampion_Gragas::ExplodeQ,
		QTravelTime + QFuseDuration,
		false
	);
}

bool AChampion_Gragas::Server_Skill_W_Validate()
{
	return true;
}

void AChampion_Gragas::Server_Skill_W_Implementation()
{
	if (!SkillComponent || !StatComponent || bEDashing) return;

	const FSkillData& WData = SkillComponent->GetW_Data();
	if (!HasCastData(WData) || !SkillComponent->TryCastSkill("W", 1)) return;

	BeginMovementLock(GetAnimationDuration(1, 0.75f));
	Multicast_PlayGragasSkillAnimation(1, 1.0f, GetActorRotation());

	bWEmpowered = true;
	bWDamageReductionActive = true;

	GetWorldTimerManager().ClearTimer(WEmpowerTimerHandle);
	GetWorldTimerManager().SetTimer(
		WEmpowerTimerHandle,
		this,
		&AChampion_Gragas::EndWEmpower,
		WEmpowerDuration,
		false
	);

	const float ReductionDuration = GetSkillValue(
		WData.Duration,
		0,
		WDamageReductionDuration
	);
	GetWorldTimerManager().ClearTimer(WDamageReductionTimerHandle);
	GetWorldTimerManager().SetTimer(
		WDamageReductionTimerHandle,
		this,
		&AChampion_Gragas::EndWDamageReduction,
		FMath::Max(ReductionDuration, 0.1f),
		false
	);
}

bool AChampion_Gragas::Server_Skill_E_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_Gragas::Server_Skill_E_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bEDashing) return;

	const FSkillData& EData = SkillComponent->GetE_Data();
	if (!HasCastData(EData) || !SkillComponent->TryCastSkill("E", 1)) return;

	const float Range = GetSkillValue(EData.Range, 0, 600.0f);
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

	bEDashing = true;
	EElapsed = 0.0f;
	Multicast_PlayGragasSkillAnimation(2, 1.0f, Direction.Rotation());
}

bool AChampion_Gragas::Server_Skill_R_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_Gragas::Server_Skill_R_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bEDashing) return;

	const FSkillData& RData = SkillComponent->GetR_Data();
	if (!HasCastData(RData) || !SkillComponent->TryCastSkill("R", 1)) return;

	const float Range = GetSkillValue(RData.Range, 0, 1000.0f);
	RExplosionLocation = ClampTargetLocation(TargetLocation, Range);

	FVector Direction = RExplosionLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;

	BeginMovementLock(GetAnimationDuration(3, 0.7f));
	Multicast_PlayGragasSkillAnimation(3, 1.0f, Direction.Rotation());

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	RExplosionLocation.Z = Start.Z;
	const float TravelTime =
		FVector::Dist2D(Start, RExplosionLocation) /
		FMath::Max(RProjectileSpeed, 1.0f);

	Multicast_SpawnGragasProjectile(
		1,
		Start,
		RExplosionLocation,
		TravelTime,
		TravelTime + 0.25f
	);

	GetWorldTimerManager().ClearTimer(RExplosionTimerHandle);
	GetWorldTimerManager().SetTimer(
		RExplosionTimerHandle,
		this,
		&AChampion_Gragas::ExplodeR,
		TravelTime,
		false
	);
}

void AChampion_Gragas::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bEDashing)
	{
		UpdateEDash(DeltaTime);
	}
}

void AChampion_Gragas::ExplodeQ()
{
	if (!HasAuthority() || !bQActive || !SkillComponent || !StatComponent) return;

	GetWorldTimerManager().ClearTimer(QExplosionTimerHandle);
	bQActive = false;
	Multicast_DestroyQProjectile();
	SpawnGragasExplosionEffect(
		QExplosionLocation,
		QExplosionEffectVisualRadius,
		true
	);
	Multicast_SpawnGragasExplosionEffect(
		QExplosionLocation,
		QExplosionEffectVisualRadius,
		true
	);

	const FSkillData& QData = SkillComponent->GetQ_Data();
	const float ChargeTime = FMath::Max(
		0.0f,
		GetWorld()->GetTimeSeconds() -
			QCastStartTime -
			QTravelTime
	);
	const float ChargeAlpha = FMath::Clamp(
		ChargeTime / FMath::Max(QFuseDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);
	const float BaseDamage = GetSkillValue(QData.BaseDamage, 0, 80.0f);
	const float SkillDamage = (
		BaseDamage +
		StatComponent->GetStat().AbilityPower * QAbilityPowerRatio
	) * (1.0f + ChargeAlpha * QMaxChargeDamageBonus);

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		QExplosionLocation,
		QExplosionLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(QExplosionRadius),
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

		ACharacter* TargetCharacter = Cast<ACharacter>(Target);
		if (!TargetCharacter) continue;

		UCharacterMovementComponent* TargetMovement =
			TargetCharacter->GetCharacterMovement();
		if (!TargetMovement) continue;

		const TWeakObjectPtr<ACharacter> TargetKey(TargetCharacter);
		if (FTimerHandle* ExistingTimer = QSlowTimerHandles.Find(TargetKey))
		{
			GetWorldTimerManager().ClearTimer(*ExistingTimer);
		}

		ULOL_StatComponent* TargetStat =
			TargetCharacter->FindComponentByClass<ULOL_StatComponent>();
		const float BaseMoveSpeed =
			TargetStat &&
			TargetStat->GetStat().MoveSpeed > 0.0f
				? TargetStat->GetStat().MoveSpeed
				: TargetMovement->MaxWalkSpeed;
		TargetMovement->MaxWalkSpeed =
			BaseMoveSpeed * (1.0f - FMath::Clamp(QSlowRatio, 0.0f, 0.9f));

		FTimerHandle& SlowTimer = QSlowTimerHandles.FindOrAdd(TargetKey);
		FTimerDelegate RestoreDelegate;
		RestoreDelegate.BindUObject(
			this,
			&AChampion_Gragas::RestoreQSlow,
			TargetKey
		);
		GetWorldTimerManager().SetTimer(
			SlowTimer,
			RestoreDelegate,
			QSlowDuration,
			false
		);
	}
}

void AChampion_Gragas::EndWEmpower()
{
	bWEmpowered = false;
}

void AChampion_Gragas::EndWDamageReduction()
{
	bWDamageReductionActive = false;
}

void AChampion_Gragas::OnBasicAttackHit(ACharacter* Target)
{
	if (!HasAuthority() || !bWEmpowered || !IsValid(Target) ||
		Target == this || !SkillComponent || !StatComponent)
	{
		return;
	}

	const FSkillData& WData = SkillComponent->GetW_Data();
	const float BaseDamage = GetSkillValue(WData.BaseDamage, 0, 20.0f);
	float TargetMaxHP = 0.0f;
	if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
	{
		if (TargetChampion->StatComponent)
		{
			TargetMaxHP = TargetChampion->StatComponent->GetStat().MaxHP;
		}
	}

	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().AbilityPower * WAbilityPowerRatio +
		TargetMaxHP * WTargetMaxHPRatio;

	UGameplayStatics::ApplyDamage(
		Target,
		SkillDamage,
		GetController(),
		this,
		ULOL_DamageMagic::StaticClass()
	);

	bWEmpowered = false;
	GetWorldTimerManager().ClearTimer(WEmpowerTimerHandle);
}

void AChampion_Gragas::UpdateEDash(float DeltaTime)
{
	EElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		EElapsed / FMath::Max(EDashDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	const FVector CurrentLocation = GetActorLocation();
	const FVector DesiredLocation =
		FMath::Lerp(EStartLocation, ETargetLocation, Alpha);

	TArray<FHitResult> ChampionHits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHitChampion = GetWorld()->SweepMultiByChannel(
		ChampionHits,
		CurrentLocation,
		DesiredLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(EHitRadius),
		QueryParams
	);

	AActor* HitTarget = nullptr;
	if (bHitChampion)
	{
		ChampionHits.Sort([](const FHitResult& A, const FHitResult& B)
		{
			return A.Distance < B.Distance;
		});

		for (const FHitResult& Hit : ChampionHits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor) || HitActor == this) continue;
			if (!HitActor->FindComponentByClass<ULOL_StateComponent>()) continue;
			if (!IsEnemyActor(HitActor)) continue;

			HitTarget = HitActor;
			break;
		}
	}

	if (IsValid(HitTarget))
	{
		FinishEDash(HitTarget);
		return;
	}

	SetActorLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (Alpha >= 1.0f)
	{
		FinishEDash(nullptr);
	}
}

void AChampion_Gragas::FinishEDash(AActor* HitTarget)
{
	if (!bEDashing) return;

	bEDashing = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (!IsValid(HitTarget) || !SkillComponent || !StatComponent) return;

	FVector EPushDirection = ETargetLocation - EStartLocation;
	EPushDirection.Z = 0.0f;
	if (EPushDirection.IsNearlyZero())
	{
		EPushDirection = HitTarget->GetActorLocation() - GetActorLocation();
		EPushDirection.Z = 0.0f;
	}

	if (ABaseChampion* HitChampion = Cast<ABaseChampion>(HitTarget))
	{
		FVector LaunchVelocity =
			EPushDirection.GetSafeNormal() * EKnockbackSpeed;
		LaunchVelocity.Z = 80.0f;
		HitChampion->StartKnockbackWithWallCheck(
			LaunchVelocity,
			EKnockbackDuration,
			0.0f
		);
	}
	else if (ACharacter* HitCharacter = Cast<ACharacter>(HitTarget))
	{
		FVector LaunchVelocity =
			EPushDirection.GetSafeNormal() * EKnockbackSpeed;
		LaunchVelocity.Z = 80.0f;
		HitCharacter->LaunchCharacter(LaunchVelocity, true, true);
	}

	const FSkillData& EData = SkillComponent->GetE_Data();
	const float BaseDamage = GetSkillValue(EData.BaseDamage, 0, 80.0f);
	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().AbilityPower * EAbilityPowerRatio;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		HitTarget->GetActorLocation(),
		HitTarget->GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(EDamageRadius),
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
		if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
		{
			TargetChampion->Multicast_ApplyStun(EStunDuration);
		}
	}
}

void AChampion_Gragas::ExplodeR()
{
	if (!HasAuthority() || !SkillComponent || !StatComponent) return;

	SpawnGragasExplosionEffect(
		RExplosionLocation,
		RExplosionEffectVisualRadius,
		false
	);
	Multicast_SpawnGragasExplosionEffect(
		RExplosionLocation,
		RExplosionEffectVisualRadius,
		false
	);

	const FSkillData& RData = SkillComponent->GetR_Data();
	const float BaseDamage = GetSkillValue(RData.BaseDamage, 0, 200.0f);
	const float SkillDamage =
		BaseDamage +
		StatComponent->GetStat().AbilityPower * RAbilityPowerRatio;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		RExplosionLocation,
		RExplosionLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(RExplosionRadius),
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

		FVector ExplosionPushDirection =
			Target->GetActorLocation() - RExplosionLocation;
		ExplosionPushDirection.Z = 0.0f;
		if (ExplosionPushDirection.IsNearlyZero())
		{
			ExplosionPushDirection =
				Target->GetActorLocation() - GetActorLocation();
			ExplosionPushDirection.Z = 0.0f;
		}

		FVector LaunchVelocity =
			ExplosionPushDirection.GetSafeNormal() * RKnockbackSpeed;
		LaunchVelocity.Z = 160.0f;
		if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
		{
			TargetChampion->StartKnockbackWithWallCheck(
				LaunchVelocity,
				RKnockbackDuration,
				0.0f
			);
		}
		else if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			TargetCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
	}
}

void AChampion_Gragas::RestoreQSlow(
	TWeakObjectPtr<ACharacter> Target)
{
	if (Target.IsValid() && Target->GetCharacterMovement())
	{
		ULOL_StatComponent* TargetStat =
			Target->FindComponentByClass<ULOL_StatComponent>();
		const float BaseMoveSpeed =
			TargetStat &&
			TargetStat->GetStat().MoveSpeed > 0.0f
				? TargetStat->GetStat().MoveSpeed
				: 330.0f;
		Target->GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	}
	QSlowTimerHandles.Remove(Target);
}

void AChampion_Gragas::BeginMovementLock(float Duration)
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
		&AChampion_Gragas::EndMovementLock,
		FMath::Max(Duration, 0.05f),
		false
	);
}

void AChampion_Gragas::EndMovementLock()
{
	bMovementLocked = false;
	if (!bEDashing && GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

bool AChampion_Gragas::IsMoveInputBlocked() const
{
	return Super::IsMoveInputBlocked() || bMovementLocked || bEDashing;
}

float AChampion_Gragas::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (HasAuthority() && bWDamageReductionActive)
	{
		DamageAmount *=
			1.0f - FMath::Clamp(WDamageReductionRatio, 0.0f, 0.9f);
	}

	return Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);
}

void AChampion_Gragas::Multicast_PlayGragasSkillAnimation_Implementation(
	uint8 SkillIndex,
	float PlayRate,
	FRotator FacingRotation)
{
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	SetActorRotation(FacingRotation);

	if (UAnimMontage* Montage = GetGragasMontage(SkillIndex))
	{
		PlayAnimMontage(Montage, PlayRate);
	}
}

void AChampion_Gragas::Multicast_SpawnGragasProjectile_Implementation(
	uint8 ProjectileType,
	FVector StartLocation,
	FVector EndLocation,
	float TravelTime,
	float LifeTime)
{
	if (!GetWorld()) return;

	UStaticMesh* ProjectileMeshAsset = ProjectileType == 0
		? QProjectileMesh.Get()
		: RProjectileMesh.Get();
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

	UStaticMeshComponent* MeshComponent =
		NewObject<UStaticMeshComponent>(
			ProjectileActor,
			TEXT("GragasProjectileMesh")
		);
	if (!MeshComponent)
	{
		ProjectileActor->Destroy();
		return;
	}

	ProjectileActor->AddInstanceComponent(MeshComponent);
	ProjectileActor->SetRootComponent(MeshComponent);
	MeshComponent->SetStaticMesh(ProjectileMeshAsset);
	if (ProjectileMaterial)
	{
		MeshComponent->SetMaterial(0, ProjectileMaterial);
	}
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->RegisterComponent();

	const float MeshRadius = ProjectileMeshAsset->GetBounds().SphereRadius;
	const float VisualRadius = ProjectileType == 0
		? QProjectileVisualRadius
		: RProjectileVisualRadius;
	const float Scale = MeshRadius > KINDA_SMALL_NUMBER
		? VisualRadius / MeshRadius
		: 1.0f;
	MeshComponent->SetWorldScale3D(FVector(Scale));
	MeshComponent->SetVisibility(true, true);
	MeshComponent->SetHiddenInGame(false, true);

	UProjectileMovementComponent* ProjectileMovement =
		NewObject<UProjectileMovementComponent>(
			ProjectileActor,
			TEXT("GragasProjectileMovement")
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
	ProjectileMovement->SetUpdatedComponent(MeshComponent);
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->InitialSpeed = VisualSpeed;
	ProjectileMovement->MaxSpeed = VisualSpeed;
	ProjectileMovement->Velocity = Direction * VisualSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->RegisterComponent();
	ProjectileMovement->Activate(true);

	TWeakObjectPtr<AActor> WeakProjectile(ProjectileActor);
	TWeakObjectPtr<UProjectileMovementComponent> WeakMovement(ProjectileMovement);
	FTimerHandle StopTimer;
	GetWorldTimerManager().SetTimer(
		StopTimer,
		FTimerDelegate::CreateLambda(
			[WeakProjectile, WeakMovement, EndLocation]()
			{
				if (WeakMovement.IsValid())
				{
					WeakMovement->StopMovementImmediately();
					WeakMovement->Deactivate();
				}
				if (WeakProjectile.IsValid())
				{
					WeakProjectile->SetActorLocation(
						EndLocation,
						false,
						nullptr,
						ETeleportType::TeleportPhysics
					);
				}
			}
		),
		FMath::Max(TravelTime, 0.01f),
		false
	);

	ProjectileActor->SetLifeSpan(FMath::Max(LifeTime, 0.2f));
	if (ProjectileType == 0)
	{
		if (IsValid(LocalQProjectileActor))
		{
			LocalQProjectileActor->Destroy();
		}
		LocalQProjectileActor = ProjectileActor;
	}
}

void AChampion_Gragas::Multicast_DestroyQProjectile_Implementation()
{
	if (IsValid(LocalQProjectileActor))
	{
		LocalQProjectileActor->Destroy();
		LocalQProjectileActor = nullptr;
	}
}

void AChampion_Gragas::Multicast_SpawnGragasExplosionEffect_Implementation(
	FVector SpawnLocation,
	float VisualRadius,
	bool bUseQEffect)
{
	if (HasAuthority()) return;

	SpawnGragasExplosionEffect(SpawnLocation, VisualRadius, bUseQEffect);
}

void AChampion_Gragas::SpawnGragasExplosionEffect(
	FVector SpawnLocation,
	float VisualRadius,
	bool bUseQEffect)
{
	if (!GetWorld())
	{
		return;
	}

	UNiagaraSystem* EffectSystem = bUseQEffect
		? QFloatEffectSystem.Get()
		: FloatEffectSystem.Get();

	if (!EffectSystem)
	{
		EffectSystem = LoadObject<UNiagaraSystem>(
			nullptr,
			bUseQEffect
				? TEXT("/Game/Level/gragas/tex_gragas/ns_gragas_float_q.ns_gragas_float_q")
				: TEXT("/Game/Level/gragas/tex_gragas/ns_gragas_float.ns_gragas_float")
		);

		if (bUseQEffect)
		{
			QFloatEffectSystem = EffectSystem;
		}
		else
		{
			FloatEffectSystem = EffectSystem;
		}
	}

	if (!EffectSystem)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Gragas explosion Niagara effect skipped: EffectSystem is still null after LoadObject. bUseQEffect=%d"),
			bUseQEffect ? 1 : 0
		);
		return;
	}

	SpawnLocation.Z += ExplosionEffectHeightOffset;

	const float NiagaraScale = FMath::Max(
		VisualRadius / FMath::Max(ExplosionEffectScaleDivisor, 1.0f),
		ExplosionEffectMinScale
	);
	UNiagaraComponent* EffectComponent =
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			EffectSystem,
			SpawnLocation,
			ExplosionEffectRotation,
			FVector(NiagaraScale),
			true,
			true,
			ENCPoolMethod::None,
			true
		);
	if (!EffectComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Gragas explosion Niagara effect spawn failed."));
		return;
	}
	EffectComponent->SetWorldScale3D(FVector(NiagaraScale));

	const TWeakObjectPtr<UNiagaraComponent> EffectWeak(EffectComponent);
	FTimerHandle DestroyEffectTimerHandle;
	GetWorldTimerManager().SetTimer(
		DestroyEffectTimerHandle,
		FTimerDelegate::CreateLambda([EffectWeak]()
		{
			if (EffectWeak.IsValid())
			{
				EffectWeak->DestroyComponent();
			}
		}),
		FMath::Max(ExplosionEffectLifeTime, 0.05f),
		false
	);

}

FVector AChampion_Gragas::ClampTargetLocation(
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

float AChampion_Gragas::GetSkillValue(
	const TArray<float>& Values,
	int32 Index,
	float Fallback) const
{
	return Values.IsValidIndex(Index) ? Values[Index] : Fallback;
}

bool AChampion_Gragas::HasCastData(const FSkillData& SkillData) const
{
	return SkillData.ManaCost.IsValidIndex(0) &&
		SkillData.Cooldown.IsValidIndex(0);
}

UAnimMontage* AChampion_Gragas::GetGragasMontage(uint8 SkillIndex) const
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

float AChampion_Gragas::GetAnimationDuration(
	uint8 SkillIndex,
	float Fallback) const
{
	if (UAnimMontage* Montage = GetGragasMontage(SkillIndex))
	{
		return FMath::Max(Montage->GetPlayLength(), 0.05f);
	}
	return Fallback;
}
