#include "Champion/Champion_Fizz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AChampion_Fizz::AChampion_Fizz()
{
	ChampionName = TEXT("Fizz");
	SetChampionData(ChampionName);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SharkMeshAsset(
		TEXT("/Game/Level/fizz/tex_fizz/fizz_r_shark.fizz_r_shark"));
	if (SharkMeshAsset.Succeeded())
	{
		SharkMesh = SharkMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> SharkAnimationAsset(
		TEXT("/Game/Level/fizz/tex_fizz/fizz_r_shark_Anim.fizz_r_shark_Anim"));
	if (SharkAnimationAsset.Succeeded())
	{
		SharkAnimation = SharkAnimationAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> RThrowMeshAsset(
		TEXT("/Game/Level/fizz/tex_fizz/fizz_r_throw.fizz_r_throw"));
	if (RThrowMeshAsset.Succeeded())
	{
		RThrowMesh = RThrowMeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RThrowMaterialAsset(
		TEXT("/Game/Level/fizz/tex_fizz/m_fizz_r.m_fizz_r"));
	if (RThrowMaterialAsset.Succeeded())
	{
		RThrowMaterial = RThrowMaterialAsset.Object;
	}
}

void AChampion_Fizz::Skill_Q()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack || bEActive) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FHitResult Hit;
	if (!PlayerController->GetHitResultUnderCursor(ECC_Pawn, false, Hit)) return;

	AActor* Target = Hit.GetActor();
	if (!IsValid(Target) || Target == this) return;
	if (!Target->FindComponentByClass<ULOL_StateComponent>() ||
		!IsEnemyActor(Target))
	{
		return;
	}

	if (GetDistanceTo(Target) <= GetQSkillRange())
	{
		bIsChasingForQ = false;
		ReservedQTarget = nullptr;
		Server_Skill_Q(Target);
		return;
	}

	ReservedQTarget = Target;
	bIsChasingForQ = true;
}

void AChampion_Fizz::Skill_W()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack || bEActive) return;
	Server_Skill_W();
}

void AChampion_Fizz::Skill_E()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (!IsValidSkillLocation(Hit.ImpactPoint)) return;
		Server_Skill_E(Hit.ImpactPoint);
	}
}

void AChampion_Fizz::Skill_R()
{
	if (!IsLocallyControlled() || bIsStunned || bIsKnockedBack || bEActive) return;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		Server_Skill_R(Hit.ImpactPoint);
	}
}

bool AChampion_Fizz::Server_Skill_Q_Validate(AActor* Target)
{
	return true;
}

void AChampion_Fizz::Server_Skill_Q_Implementation(AActor* Target)
{
	if (!SkillComponent || !StatComponent || !IsValid(Target) || Target == this) return;
	if (!Target->FindComponentByClass<ULOL_StateComponent>() ||
		!IsEnemyActor(Target))
	{
		return;
	}
	if (bIsQDashing || bEActive || GetDistanceTo(Target) > GetQSkillRange() + 50.0f) return;
	if (!SkillComponent->TryCastSkill("Q", 1)) return;

	if (AttackComponent)
	{
		AttackComponent->CancelAttack();
		AttackComponent->CombatTarget = nullptr;
		AttackComponent->HitTarget = nullptr;
	}

	FVector Direction = Target->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	const FRotator FacingRotation = Direction.IsNearlyZero()
		? GetActorRotation()
		: Direction.Rotation();

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	Multicast_PlayFizzSkillAnimation(0, 0, 1.0f, FacingRotation);

	QDashStart = GetActorLocation();
	QDashTarget = Target;
	QDashElapsed = 0.0f;
	bIsQDashing = true;

	// Q must pass through the target capsule to finish on its far side.
	GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
	if (UCapsuleComponent* TargetCapsule =
		Target->FindComponentByClass<UCapsuleComponent>())
	{
		TargetCapsule->IgnoreActorWhenMoving(this, true);
	}
}

bool AChampion_Fizz::Server_Skill_W_Validate()
{
	return true;
}

void AChampion_Fizz::Server_Skill_W_Implementation()
{
	if (!SkillComponent || !AttackComponent || bEActive) return;
	if (!SkillComponent->TryCastSkill("W", 1)) return;

	bWEmpowered = true;

	// Fizz W is an empowered basic attack. Use W montage when supplied,
	// otherwise use the normal attack montage as its activation animation.
	Multicast_PlayFizzSkillAnimation(1, 0, 1.15f, GetActorRotation());

	GetWorldTimerManager().ClearTimer(AttackComponent->AttackTimerHandle);
	AttackComponent->ResetAttack();

	const FSkillData& WData = SkillComponent->GetW_Data();
	const float Duration = GetSkillValue(WData.Duration, 0, 5.0f);
	GetWorldTimerManager().ClearTimer(WEmpowerTimerHandle);
	GetWorldTimerManager().SetTimer(
		WEmpowerTimerHandle,
		this,
		&AChampion_Fizz::EndWEmpower,
		Duration > 0.0f ? Duration : 5.0f,
		false
	);
}

bool AChampion_Fizz::Server_Skill_E_Validate(FVector TargetLocation)
{
	return !TargetLocation.ContainsNaN();
}

void AChampion_Fizz::Server_Skill_E_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent) return;
	if (!IsValidSkillLocation(TargetLocation)) return;

	const FSkillData& EData = SkillComponent->GetE_Data();
	const float Range = GetSkillValue(EData.Range, 0, 400.0f);

	// A second E input during the ascent starts the descent immediately.
	if (bEActive)
	{
		if (!bEDescending)
		{
			ETargetLocation = ClampTargetLocation(TargetLocation, Range);
			if (!IsValidSkillLocation(ETargetLocation)) return;
			BeginEDescent();
		}
		return;
	}

	if (!SkillComponent->TryCastSkill("E", 1)) return;

	ETargetLocation = ClampTargetLocation(TargetLocation, Range);
	if (!IsValidSkillLocation(ETargetLocation)) return;
	bEActive = true;
	bEDescending = false;

	if (AttackComponent)
	{
		AttackComponent->CancelAttack();
		AttackComponent->CombatTarget = nullptr;
		AttackComponent->HitTarget = nullptr;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	FVector Direction = ETargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	const FRotator FacingRotation = Direction.IsNearlyZero()
		? GetActorRotation()
		: Direction.Rotation();

	// First E animation: EMontage[0] (Playful/ascent).
	Multicast_PlayFizzSkillAnimation(2, 0, 1.0f, FacingRotation);

	const float Duration = GetSkillValue(EData.Duration, 0, 1.2f);
	const float AscentDuration = FMath::Max(0.1f, Duration - EDescentDuration);
	GetWorldTimerManager().ClearTimer(EAscentTimerHandle);
	GetWorldTimerManager().SetTimer(
		EAscentTimerHandle,
		this,
		&AChampion_Fizz::BeginEDescent,
		AscentDuration,
		false
	);
}

bool AChampion_Fizz::Server_Skill_R_Validate(FVector TargetLocation)
{
	return true;
}

void AChampion_Fizz::Server_Skill_R_Implementation(FVector TargetLocation)
{
	if (!SkillComponent || !StatComponent || bEActive) return;
	if (!SkillComponent->TryCastSkill("R", 1)) return;

	const FSkillData& RData = SkillComponent->GetR_Data();
	const float Range = GetSkillValue(RData.Range, 0, 1300.0f);

	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;
	Direction.Normalize();

	const FRotator FacingRotation = Direction.Rotation();
	Multicast_PlayFizzSkillAnimation(3, 0, 1.0f, FacingRotation);

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	FVector End = ClampTargetLocation(TargetLocation, Range);
	End.Z = Start.Z;

	RAttachedTarget = nullptr;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(RProjectileRadius),
		QueryParams
	);

	FHitResult TargetHit;
	bool bFoundValidTarget = false;
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
			if (!HitActor->FindComponentByClass<ULOL_StateComponent>() ||
				!IsEnemyActor(HitActor))
			{
				continue;
			}

			TargetHit = Hit;
			RAttachedTarget = Cast<ABaseChampion>(HitActor);
			bFoundValidTarget = true;
			break;
		}
	}

	// Keep the visual projectile on the cursor ray. Using the target actor's
	// center here can bend the projectile toward an enemy caught by the sweep.
	const float MaxProjectileDistance = FVector::Dist2D(Start, End);
	const float ProjectileDistance = bFoundValidTarget
		? FMath::Clamp(TargetHit.Distance, 0.0f, MaxProjectileDistance)
		: MaxProjectileDistance;
	const FVector ProjectileEnd = Start + Direction * ProjectileDistance;

	RExplosionLocation = RAttachedTarget
		? RAttachedTarget->GetActorLocation()
		: ProjectileEnd;

	const float TravelTime =
		FVector::Dist2D(Start, ProjectileEnd) /
		FMath::Max(RProjectileSpeed, 1.0f);
	Multicast_SpawnRProjectile(Start, ProjectileEnd, TravelTime);

	const float Delay = GetSkillValue(RData.Duration, 0, 2.0f);
	GetWorldTimerManager().ClearTimer(RExplosionTimerHandle);
	GetWorldTimerManager().SetTimer(
		RExplosionTimerHandle,
		this,
		&AChampion_Fizz::ExplodeChumTheWaters,
		TravelTime + (Delay > 0.0f ? Delay : 2.0f),
		false
	);
}

void AChampion_Fizz::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled() && bIsChasingForQ)
	{
		UpdateQChaseToCast();
	}

	if (HasAuthority() && bIsQDashing)
	{
		UpdateQDash(DeltaTime);
	}

	if (HasAuthority() && bEDescending)
	{
		UpdateEDescent(DeltaTime);
	}
}

void AChampion_Fizz::UpdateQChaseToCast()
{
	if (bIsStunned || bIsKnockedBack || !IsValid(ReservedQTarget))
	{
		bIsChasingForQ = false;
		ReservedQTarget = nullptr;
		return;
	}

	if (GetDistanceTo(ReservedQTarget) <= GetQSkillRange())
	{
		AActor* Target = ReservedQTarget;
		bIsChasingForQ = false;
		ReservedQTarget = nullptr;

		if (MoveComponent)
		{
			MoveComponent->StopMovement();
		}
		Server_Skill_Q(Target);
		return;
	}

	if (MoveComponent)
	{
		MoveComponent->TargetLocation = ReservedQTarget->GetActorLocation();
	}

	FVector Direction = ReservedQTarget->GetActorLocation() - GetActorLocation();
	Direction.Z = 0.0f;
	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction.GetSafeNormal(), 1.0f);
	}
}

void AChampion_Fizz::UpdateQDash(float DeltaTime)
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

	float TargetRadius = 0.0f;
	if (UCapsuleComponent* TargetCapsule =
		QDashTarget->FindComponentByClass<UCapsuleComponent>())
	{
		TargetRadius = TargetCapsule->GetScaledCapsuleRadius();
	}
	const float TargetOffset =
		GetCapsuleComponent()->GetScaledCapsuleRadius() +
		TargetRadius +
		QBehindTargetDistance;
	FVector DashEnd = QDashTarget->GetActorLocation() +
		Direction.GetSafeNormal() * TargetOffset;
	DashEnd.Z = QDashStart.Z;

	SetActorLocation(
		FMath::Lerp(QDashStart, DashEnd, Alpha),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (Alpha >= 1.0f)
	{
		FinishQDash();
	}
}

void AChampion_Fizz::FinishQDash()
{
	if (!bIsQDashing) return;

	AActor* Target = QDashTarget;
	bIsQDashing = false;
	QDashTarget = nullptr;

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

	if (!IsValid(Target) || !SkillComponent || !StatComponent) return;

	const FSkillData& QData = SkillComponent->GetQ_Data();
	const float BaseDamage = GetSkillValue(QData.BaseDamage, 0, 10.0f);
	const float MagicDamage =
		BaseDamage + StatComponent->GetStat().AbilityPower * QAbilityPowerRatio;

	// Q applies one basic-attack hit plus its own magic damage.
	UGameplayStatics::ApplyDamage(
		Target,
		StatComponent->GetStat().AttackDamage,
		GetController(),
		this,
		ULOL_DamagePhysical::StaticClass()
	);
	UGameplayStatics::ApplyDamage(
		Target,
		MagicDamage,
		GetController(),
		this,
		ULOL_DamageMagic::StaticClass()
	);

	if (ACharacter* CharacterTarget = Cast<ACharacter>(Target))
	{
		OnBasicAttackHit(CharacterTarget);
	}
}

void AChampion_Fizz::OnBasicAttackHit(ACharacter* Target)
{
	if (!HasAuthority() || !IsValid(Target) || Target == this ||
		!SkillComponent || !StatComponent)
	{
		return;
	}

	StartWPassiveBleed(Target);

	if (!bWEmpowered) return;

	const FSkillData& WData = SkillComponent->GetW_Data();
	const float BaseDamage = GetSkillValue(WData.BaseDamage, 0, 50.0f);
	const float SkillDamage =
		BaseDamage + StatComponent->GetStat().AbilityPower * WActiveAbilityPowerRatio;

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

void AChampion_Fizz::StartWPassiveBleed(ACharacter* Target)
{
	if (!IsValid(Target) || !SkillComponent || !StatComponent) return;

	const TWeakObjectPtr<ACharacter> TargetKey(Target);
	if (FTimerHandle* ExistingTimer = WBleedTimers.Find(TargetKey))
	{
		GetWorldTimerManager().ClearTimer(*ExistingTimer);
	}

	WBleedTicks.FindOrAdd(TargetKey) = 0;
	FTimerHandle& BleedTimer = WBleedTimers.FindOrAdd(TargetKey);

	GetWorldTimerManager().SetTimer(
		BleedTimer,
		FTimerDelegate::CreateWeakLambda(this, [this, TargetKey]()
		{
			if (!TargetKey.IsValid() || !SkillComponent || !StatComponent)
			{
				if (FTimerHandle* Timer = WBleedTimers.Find(TargetKey))
				{
					GetWorldTimerManager().ClearTimer(*Timer);
				}
				WBleedTimers.Remove(TargetKey);
				WBleedTicks.Remove(TargetKey);
				return;
			}

			int32& CurrentTick = WBleedTicks.FindOrAdd(TargetKey);
			CurrentTick++;

			const FSkillData& WData = SkillComponent->GetW_Data();
			const float PassiveBaseDamage = GetSkillValue(WData.SecondaryValue, 0, 20.0f);
			const float TotalDamage =
				PassiveBaseDamage +
				StatComponent->GetStat().AbilityPower * WPassiveAbilityPowerRatio;
			const float TickDamage =
				TotalDamage / FMath::Max(1, WPassiveTickCount);

			UGameplayStatics::ApplyDamage(
				TargetKey.Get(),
				TickDamage,
				GetController(),
				this,
				ULOL_DamageMagic::StaticClass()
			);

			if (CurrentTick >= WPassiveTickCount)
			{
				if (FTimerHandle* Timer = WBleedTimers.Find(TargetKey))
				{
					GetWorldTimerManager().ClearTimer(*Timer);
				}
				WBleedTimers.Remove(TargetKey);
				WBleedTicks.Remove(TargetKey);
			}
		}),
		1.0f,
		true
	);
}

void AChampion_Fizz::EndWEmpower()
{
	bWEmpowered = false;
}

void AChampion_Fizz::BeginEDescent()
{
	if (!HasAuthority() || !bEActive || bEDescending) return;
	if (!IsValidSkillLocation(ETargetLocation))
	{
		ResetPlayfulTricksterState(true);
		return;
	}

	bEDescending = true;
	EDescentStartLocation = GetActorLocation();
	if (!IsValidSkillLocation(EDescentStartLocation))
	{
		ResetPlayfulTricksterState(true);
		return;
	}
	EDescentElapsed = 0.0f;
	GetWorldTimerManager().ClearTimer(EAscentTimerHandle);

	FVector Direction = ETargetLocation - GetActorLocation();
	Direction.Z = 0.0f;
	const FRotator FacingRotation = Direction.IsNearlyZero()
		? GetActorRotation()
		: Direction.Rotation();

	// Second E animation: EMontage[1] (Trickster/descent).
	Multicast_PlayFizzSkillAnimation(2, 1, 1.0f, FacingRotation);

	GetWorldTimerManager().ClearTimer(EDescentTimerHandle);
	GetWorldTimerManager().SetTimer(
		EDescentTimerHandle,
		this,
		&AChampion_Fizz::FinishPlayfulTrickster,
		EDescentDuration,
		false
	);
}

void AChampion_Fizz::UpdateEDescent(float DeltaTime)
{
	if (!bEActive || !bEDescending) return;
	if (!IsValidSkillLocation(EDescentStartLocation) ||
		!IsValidSkillLocation(ETargetLocation))
	{
		ResetPlayfulTricksterState(true);
		return;
	}

	EDescentElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(
		EDescentElapsed / FMath::Max(EDescentDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f
	);

	const FVector NewLocation =
		FMath::Lerp(EDescentStartLocation, ETargetLocation, Alpha);
	if (!IsValidSkillLocation(NewLocation))
	{
		ResetPlayfulTricksterState(true);
		return;
	}

	SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.0f)
	{
		FinishPlayfulTrickster();
	}
}

void AChampion_Fizz::FinishPlayfulTrickster()
{
	if (!HasAuthority() || !bEActive) return;

	GetWorldTimerManager().ClearTimer(EDescentTimerHandle);
	GetWorldTimerManager().ClearTimer(EAscentTimerHandle);

	if (!IsValidSkillLocation(ETargetLocation))
	{
		ResetPlayfulTricksterState(true);
		return;
	}

	SetActorLocation(ETargetLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (!SkillComponent || !StatComponent)
	{
		ResetPlayfulTricksterState(true);
		return;
	}

	const FSkillData& EData = SkillComponent->GetE_Data();
	const float BaseDamage = GetSkillValue(EData.BaseDamage, 0, 70.0f);
	const float SkillDamage =
		BaseDamage + StatComponent->GetStat().AbilityPower * EAbilityPowerRatio;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		GetActorLocation(),
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(EDamageRadius),
		QueryParams
	);

	if (bHit)
	{
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
		}
	}

	ResetPlayfulTricksterState(true);
}

void AChampion_Fizz::ResetPlayfulTricksterState(bool bRestoreMovement)
{
	GetWorldTimerManager().ClearTimer(EAscentTimerHandle);
	GetWorldTimerManager().ClearTimer(EDescentTimerHandle);

	bEActive = false;
	bEDescending = false;
	EDescentElapsed = 0.0f;
	EDescentStartLocation = FVector::ZeroVector;
	ETargetLocation = FVector::ZeroVector;

	if (bRestoreMovement)
	{
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
}

void AChampion_Fizz::ExplodeChumTheWaters()
{
	if (!HasAuthority() || !SkillComponent || !StatComponent) return;

	const FVector ExplosionLocation = IsValid(RAttachedTarget)
		? RAttachedTarget->GetActorLocation()
		: RExplosionLocation;
	RAttachedTarget = nullptr;

	FVector SharkDirection = ExplosionLocation - GetActorLocation();
	SharkDirection.Z = 0.0f;
	const FRotator SharkRotation = SharkDirection.IsNearlyZero()
		? GetActorRotation()
		: SharkDirection.Rotation();
	Multicast_SpawnSharkEffect(ExplosionLocation, SharkRotation);

	const FSkillData& RData = SkillComponent->GetR_Data();
	const float BaseDamage = GetSkillValue(RData.BaseDamage, 0, 150.0f);
	const float SkillDamage =
		BaseDamage + StatComponent->GetStat().AbilityPower * RAbilityPowerRatio;

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		Hits,
		ExplosionLocation,
		ExplosionLocation,
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
		if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
		{
			TargetChampion->Multicast_ApplyStun(RStunDuration);
		}
	}
}

void AChampion_Fizz::Multicast_SpawnSharkEffect_Implementation(
	FVector SpawnLocation,
	FRotator SpawnRotation)
{
	if (!SharkMesh || !SharkAnimation || !GetWorld()) return;

	SpawnLocation.Z += RSharkZOffset;
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SharkActor = GetWorld()->SpawnActor<AActor>(
		AActor::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParameters
	);
	if (!SharkActor) return;

	USkeletalMeshComponent* SharkComponent = NewObject<USkeletalMeshComponent>(
		SharkActor,
		TEXT("FizzSharkMesh")
	);
	if (!SharkComponent)
	{
		SharkActor->Destroy();
		return;
	}

	SharkActor->AddInstanceComponent(SharkComponent);
	SharkActor->SetRootComponent(SharkComponent);
	SharkComponent->SetSkeletalMesh(SharkMesh);
	SharkComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SharkComponent->SetGenerateOverlapEvents(false);
	SharkComponent->SetCastShadow(false);
	SharkComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SharkComponent->RegisterComponent();

	// The actor was spawned before it had a root component, so apply the
	// explosion transform again after the skeletal mesh becomes the root.
	SharkActor->SetActorLocationAndRotation(
		SpawnLocation,
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
	SharkComponent->SetWorldScale3D(FVector(RSharkScale));
	SharkComponent->SetVisibility(true, true);
	SharkComponent->SetHiddenInGame(false, true);
	SharkComponent->PlayAnimation(SharkAnimation, false);

	SharkActor->SetLifeSpan(FMath::Max(SharkAnimation->GetPlayLength() + 0.2f, 1.0f));
}

void AChampion_Fizz::Multicast_SpawnRProjectile_Implementation(
	FVector StartLocation,
	FVector EndLocation,
	float TravelTime)
{
	if (!RThrowMesh || !GetWorld()) return;

	const FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
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

	UStaticMeshComponent* ProjectileMesh = NewObject<UStaticMeshComponent>(
		ProjectileActor,
		TEXT("FizzRThrowMesh")
	);
	if (!ProjectileMesh)
	{
		ProjectileActor->Destroy();
		return;
	}

	ProjectileActor->AddInstanceComponent(ProjectileMesh);
	ProjectileActor->SetRootComponent(ProjectileMesh);
	ProjectileMesh->SetStaticMesh(RThrowMesh);
	if (RThrowMaterial)
	{
		ProjectileMesh->SetMaterial(0, RThrowMaterial);
	}
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
	ProjectileMesh->SetRelativeRotation(RProjectileRotationOffset);

	const float MeshRadius = RThrowMesh->GetBounds().SphereRadius;
	const float BoundsScale = MeshRadius > KINDA_SMALL_NUMBER
		? RProjectileVisualRadius / MeshRadius
		: 1.0f;
	ProjectileMesh->SetWorldScale3D(FVector(BoundsScale * RProjectileScale));
	ProjectileMesh->SetVisibility(true, true);
	ProjectileMesh->SetHiddenInGame(false, true);

	UProjectileMovementComponent* ProjectileMovement =
		NewObject<UProjectileMovementComponent>(
			ProjectileActor,
			TEXT("FizzRProjectileMovement")
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

	ProjectileActor->SetLifeSpan(FMath::Max(TravelTime + 0.1f, 0.2f));
}

float AChampion_Fizz::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (HasAuthority() && bEActive)
	{
		return 0.0f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

FVector AChampion_Fizz::ClampTargetLocation(FVector TargetLocation, float MaxRange) const
{
	const FVector Start = GetActorLocation();
	if (!IsValidSkillLocation(TargetLocation) ||
		!IsValidSkillLocation(Start) ||
		MaxRange <= 0.0f)
	{
		return Start;
	}

	FVector Direction = TargetLocation - Start;
	Direction.Z = 0.0f;

	const float Distance = Direction.Size();
	if (Distance > MaxRange)
	{
		Direction = Direction.GetSafeNormal() * MaxRange;
	}

	FVector Result = Start + Direction;
	Result.Z = Start.Z;
	return Result;
}

bool AChampion_Fizz::IsValidSkillLocation(const FVector& Location) const
{
	return
		!Location.ContainsNaN() &&
		FMath::IsFinite(Location.X) &&
		FMath::IsFinite(Location.Y) &&
		FMath::IsFinite(Location.Z);
}

float AChampion_Fizz::GetSkillValue(
	const TArray<float>& Values,
	int32 Index,
	float Fallback) const
{
	return Values.IsValidIndex(Index) ? Values[Index] : Fallback;
}

float AChampion_Fizz::GetQSkillRange() const
{
	if (!SkillComponent) return 550.0f;
	return GetSkillValue(SkillComponent->GetQ_Data().Range, 0, 550.0f);
}

UAnimMontage* AChampion_Fizz::GetFizzMontage(uint8 SkillIndex, int32 MontageIndex) const
{
	const TArray<UAnimMontage*>* Montages = nullptr;

	switch (SkillIndex)
	{
	case 0:
		Montages = &ChampionResource.QMontage;
		break;
	case 1:
		Montages = ChampionResource.WMontage.Num() > 0
			? &ChampionResource.WMontage
			: &ChampionResource.AttackMontage;
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

void AChampion_Fizz::Multicast_PlayFizzSkillAnimation_Implementation(
	uint8 SkillIndex,
	int32 MontageIndex,
	float PlayRate,
	FRotator FacingRotation)
{
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	SetActorRotation(FacingRotation);

	if (UAnimMontage* Montage = GetFizzMontage(SkillIndex, MontageIndex))
	{
		PlayAnimMontage(Montage, PlayRate);
	}
}
