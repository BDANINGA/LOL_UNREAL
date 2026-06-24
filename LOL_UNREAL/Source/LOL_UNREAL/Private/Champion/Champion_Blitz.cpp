#include "Champion/Champion_Blitz.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h" 


#include "UObject/ConstructorHelpers.h"

AChampion_Blitz::AChampion_Blitz()
{
	ChampionName = TEXT("Blitz");
	SetChampionData(ChampionName);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> QProjectileMeshAsset(
        TEXT("/Game/Level/blitzcrank/blitz_tex/blitz_q.blitz_q")
    );
    if (QProjectileMeshAsset.Succeeded())
    {
        QProjectileMesh = QProjectileMeshAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Blitz Q projectile mesh failed to load."));
    }

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 330.0f;
	}
}

void AChampion_Blitz::Skill_Q()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult HitResult;
    if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

    Server_Skill_Q(HitResult.ImpactPoint);
}


bool AChampion_Blitz::Server_Skill_Q_Validate(FVector TargetLocation)
{
    return true;
}

void AChampion_Blitz::Server_Skill_Q_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !GetWorld()) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;

    if (!QData.Range.IsValidIndex(SkillLevelIdx) ||
        !QData.Cooldown.IsValidIndex(SkillLevelIdx))
    {
        UE_LOG(LogTemp, Error, TEXT("Blitz Q skill data not loaded. RangeNum=%d CooldownNum=%d"),
            QData.Range.Num(),
            QData.Cooldown.Num());
        return;
    }

    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FVector LookDirection = TargetLocation - GetActorLocation();
    LookDirection.Z = 0.0f;
    if (LookDirection.IsNearlyZero()) return;

    const FRotator LookRotation = LookDirection.Rotation();
    const FVector SkillDirection = LookRotation.Vector();

    SetActorRotation(LookRotation);

    if (ChampionResource.QMontage.IsValidIndex(AM_SKIll_Q_IDX) &&
        ChampionResource.QMontage[AM_SKIll_Q_IDX])
    {
        Multicast_SetTargetAndPlayMontage(
            ChampionResource.QMontage[AM_SKIll_Q_IDX],
            1.0f,
            LookRotation
        );
    }

    const float QRange = QData.Range[SkillLevelIdx];
    const float HookRadius = Q_Radius > 0.0f ? Q_Radius : 70.0f;

    const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 80.f);
    const FVector TraceEnd = TraceStart + SkillDirection * QRange;
    const FVector VisualStart = TraceStart;
    const FVector VisualMaxEnd = VisualStart + SkillDirection * QRange;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(HookRadius),
        Params
    );

    ACharacter* Target = nullptr;
    FVector VisualEnd = VisualMaxEnd;

    if (bHit)
    {
        Hits.Sort([](const FHitResult& A, const FHitResult& B)
        {
            return A.Distance < B.Distance;
        });

        for (const FHitResult& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!IsValidBlitzSkillTarget(HitActor)) continue;

            Target = Cast<ACharacter>(HitActor);
            const float HitDistanceAlongPath = FMath::Clamp(
                FVector::DotProduct(HitActor->GetActorLocation() - TraceStart, SkillDirection),
                0.0f,
                QRange
            );
            VisualEnd = VisualStart + SkillDirection * HitDistanceAlongPath;
            break;
        }
    }

    const float TravelDistance = FVector::Dist(VisualStart, VisualEnd);
    const float TravelTime = TravelDistance / FMath::Max(QProjectileSpeed, 1.0f);

    Multicast_SpawnQProjectileVisual(VisualStart, VisualEnd, TravelTime);

    if (!Target)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(PullTimerHandle);
    GetWorldTimerManager().ClearTimer(PullTimeoutTimerHandle);

    FTimerHandle BeginPullTimerHandle;
    GetWorldTimerManager().SetTimer(
        BeginPullTimerHandle,
        FTimerDelegate::CreateUObject(this, &AChampion_Blitz::BeginPullTarget, Target, SkillDirection),
        FMath::Max(TravelTime, 0.01f),
        false
    );
}
void AChampion_Blitz::TickPullTarget()
{
    if (!IsValid(GrabbedTarget))
    {
        RestoreGrabbedTargetCollision();
        GetWorldTimerManager().ClearTimer(PullTimerHandle);
        GrabbedTarget = nullptr;
        return;
    }

    FVector CurrentLocation = GrabbedTarget->GetActorLocation();

    float DistanceToDest = FVector::Dist2D(PullDestination, CurrentLocation);

    if (DistanceToDest <= 20.0f)
    {
        GrabbedTarget->SetActorLocation(PullDestination, false, nullptr, ETeleportType::TeleportPhysics);

        if (GrabbedTarget->GetCharacterMovement())
        {
            GrabbedTarget->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }

        FRotator FinalRotation = (-GetActorForwardVector()).Rotation();
        FinalRotation.Pitch = 0.f;
        FinalRotation.Roll = 0.f;
        GrabbedTarget->SetActorRotation(FinalRotation);

        RestoreGrabbedTargetCollision();
        GetWorldTimerManager().ClearTimer(PullTimerHandle);
        GrabbedTarget = nullptr;
        return;
    }

    FVector NewLocation = FMath::VInterpConstantTo(
        CurrentLocation,
        PullDestination,
        0.01f,
        PullSpeed * 220.0f
    );

    GrabbedTarget->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

    FRotator SmoothRotation = (-GetActorForwardVector()).Rotation();
    SmoothRotation.Pitch = 0.f;
    SmoothRotation.Roll = 0.f;
    GrabbedTarget->SetActorRotation(SmoothRotation);
}
void AChampion_Blitz::FinishPullTarget()
{
    GetWorldTimerManager().ClearTimer(PullTimerHandle);
    GetWorldTimerManager().ClearTimer(PullTimeoutTimerHandle);

    if (GrabbedTarget && GrabbedTarget->GetCharacterMovement())
    {
        GrabbedTarget->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    RestoreGrabbedTargetCollision();

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    GrabbedTarget = nullptr;
}

void AChampion_Blitz::BeginPullTarget(ACharacter* Target, FVector SkillDirection)
{
    if (!HasAuthority() || !IsValidBlitzSkillTarget(Target)) return;

    GrabbedTarget = Target;

    PullDestination = GetActorLocation() + GetActorForwardVector().GetSafeNormal2D() * Q_PullDistance;
    PullDestination.Z = GrabbedTarget->GetActorLocation().Z;

    if (UCapsuleComponent* Capsule = GrabbedTarget->GetCapsuleComponent())
    {
        GrabbedTargetPreviousCollisionEnabled = Capsule->GetCollisionEnabled();
        GrabbedTargetPreviousPawnResponse = Capsule->GetCollisionResponseToChannel(ECC_Pawn);
        Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (GrabbedTarget->GetCharacterMovement())
    {
        GrabbedTarget->GetCharacterMovement()->StopMovementImmediately();
        GrabbedTarget->GetCharacterMovement()->DisableMovement();
    }

    GetWorldTimerManager().SetTimer(
        PullTimerHandle,
        this,
        &AChampion_Blitz::TickPullTarget,
        0.01f,
        true
    );

    GetWorldTimerManager().SetTimer(
        PullTimeoutTimerHandle,
        this,
        &AChampion_Blitz::FinishPullTarget,
        0.8f,
        false
    );
}

void AChampion_Blitz::RestoreGrabbedTargetCollision()
{
    if (GrabbedTarget)
    {
        if (UCapsuleComponent* Capsule = GrabbedTarget->GetCapsuleComponent())
        {
            Capsule->SetCollisionResponseToChannel(ECC_Pawn, GrabbedTargetPreviousPawnResponse);
            Capsule->SetCollisionEnabled(GrabbedTargetPreviousCollisionEnabled);
        }
    }
}

bool AChampion_Blitz::IsValidBlitzSkillTarget(AActor* TargetActor) const
{
    if (!IsValid(TargetActor) || TargetActor == this) return false;

    const bool bSupportedTarget =
        Cast<ABaseChampion>(TargetActor) ||
        Cast<ABaseMinion>(TargetActor) ||
        Cast<ABaseJungleMonster>(TargetActor);
    if (!bSupportedTarget) return false;

    ULOL_StateComponent* TargetState = TargetActor->FindComponentByClass<ULOL_StateComponent>();
    if (!TargetState || TargetState->HasStatusTag(LOLTags::State_Dead)) return false;

    return IsEnemyActor(TargetActor);
}

void AChampion_Blitz::Multicast_SpawnQProjectileVisual_Implementation(FVector StartLocation, FVector EndLocation, float TravelTime)
{
    SpawnQProjectileVisual(StartLocation, EndLocation, TravelTime);
}

void AChampion_Blitz::SpawnQProjectileVisual(FVector StartLocation, FVector EndLocation, float TravelTime)
{
    if (!GetWorld()) return;

    if (!QProjectileMesh)
    {
        QProjectileMesh = LoadObject<UStaticMesh>(
            nullptr,
            TEXT("/Game/Level/blitzcrank/blitz_tex/blitz_q.blitz_q")
        );
    }

    if (!QProjectileMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Blitz Q projectile mesh missing."));
        return;
    }

    const FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
    if (Direction.IsNearlyZero()) return;

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* ProjectileActor = GetWorld()->SpawnActor<AActor>(
        AActor::StaticClass(),
        StartLocation,
        Direction.Rotation(),
        SpawnParameters
    );
    if (!ProjectileActor) return;
    ProjectileActor->SetActorHiddenInGame(false);

    USphereComponent* CollisionComponent = NewObject<USphereComponent>(
        ProjectileActor,
        TEXT("BlitzQProjectileRoot")
    );
    if (!CollisionComponent)
    {
        ProjectileActor->Destroy();
        return;
    }

    ProjectileActor->AddInstanceComponent(CollisionComponent);
    ProjectileActor->SetRootComponent(CollisionComponent);
    CollisionComponent->SetMobility(EComponentMobility::Movable);
    CollisionComponent->InitSphereRadius(Q_Radius > 0.0f ? Q_Radius : 50.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CollisionComponent->SetGenerateOverlapEvents(false);
    CollisionComponent->RegisterComponent();

    UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(
        ProjectileActor,
        TEXT("BlitzQProjectileMesh")
    );
    if (!MeshComponent)
    {
        ProjectileActor->Destroy();
        return;
    }

    ProjectileActor->AddInstanceComponent(MeshComponent);
    MeshComponent->SetupAttachment(CollisionComponent);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(QProjectileMesh);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
    MeshComponent->RegisterComponent();
    MeshComponent->SetRelativeRotation(QProjectileRotationOffset);

    ProjectileActor->SetActorLocationAndRotation(
        StartLocation,
        Direction.Rotation(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    const float MeshRadius = QProjectileMesh->GetBounds().SphereRadius;
    const float MeshScale = MeshRadius > KINDA_SMALL_NUMBER
        ? QProjectileVisualRadius / MeshRadius
        : 1.0f;
    MeshComponent->SetWorldScale3D(FVector(MeshScale * QProjectileVisualScale));
    MeshComponent->SetVisibility(true, true);
    MeshComponent->SetHiddenInGame(false, true);

    UProjectileMovementComponent* ProjectileMovement = NewObject<UProjectileMovementComponent>(
        ProjectileActor,
        TEXT("BlitzQProjectileMovement")
    );
    if (!ProjectileMovement)
    {
        ProjectileActor->Destroy();
        return;
    }

    const float VisualSpeed = FVector::Dist(StartLocation, EndLocation) / FMath::Max(TravelTime, KINDA_SMALL_NUMBER);
    ProjectileActor->AddInstanceComponent(ProjectileMovement);
    ProjectileMovement->SetUpdatedComponent(CollisionComponent);
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->InitialSpeed = VisualSpeed;
    ProjectileMovement->MaxSpeed = VisualSpeed;
    ProjectileMovement->Velocity = Direction * VisualSpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = false;
    ProjectileMovement->RegisterComponent();
    ProjectileMovement->Activate(true);

    ProjectileActor->SetLifeSpan(FMath::Max(TravelTime, 0.05f));

    UE_LOG(LogTemp, Warning, TEXT("Blitz Q projectile spawned. Mesh=%s Start=%s End=%s TravelTime=%.2f Scale=%.2f"),
        *QProjectileMesh->GetName(),
        *StartLocation.ToString(),
        *EndLocation.ToString(),
        TravelTime,
        MeshScale * QProjectileVisualScale);
}
void AChampion_Blitz::Skill_W() 
{
	if (!IsLocallyControlled()) return;
	if (bIsStunned || bIsKnockedBack) return;

	Server_Skill_W();
}

bool AChampion_Blitz::Server_Skill_W_Validate() { return true; }

void AChampion_Blitz::Server_Skill_W_Implementation()
{
    
    GetWorldTimerManager().ClearTimer(W_BuffTimerHandle);
    GetWorldTimerManager().ClearTimer(W_SlowTimerHandle);

    if (GetCharacterMovement())
    { 
        GetCharacterMovement()->MaxWalkSpeed = 330.0f + (330.0f * W_SpeedBuffAmount);
    }

    GetWorldTimerManager().SetTimer(W_BuffTimerHandle, this, &AChampion_Blitz::EndWBuff, W_Duration, false);
}

void AChampion_Blitz::EndWBuff()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f - (330.0f * W_SlowAmount);
    }

    UE_LOG(LogTemp, Warning, TEXT("[Server Blitz W] End W Buff - Slow Start"));

    GetWorldTimerManager().SetTimer(W_SlowTimerHandle, this, &AChampion_Blitz::EndWSlow, W_SlowDuration, false);
}

void AChampion_Blitz::EndWSlow()
{
    if (!HasAuthority()) return;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = 330.0f;
    }
}

// ==========================================
// E 
// ==========================================
void AChampion_Blitz::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_E();
}

bool AChampion_Blitz::Server_Skill_E_Validate() { return true; }

void AChampion_Blitz::Server_Skill_E_Implementation()
{
    if (!SkillComponent) return;
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    bIsEActive = true;

    UE_LOG(LogTemp, Warning, TEXT("Blitz E Active"));
}

void AChampion_Blitz::ResetE()
{
    bIsEActive = false;
}

void AChampion_Blitz::OnAttackHitWithE(ACharacter* Target)
{
    if (!HasAuthority() || !Target) return;
    if (!bIsEActive) return;
    if (!IsValidBlitzSkillTarget(Target)) return;

    UE_LOG(LogTemp, Warning, TEXT("Blitz E KnockUp"));

    Target->StopAnimMontage();

    if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->CurrentRootMotion.Clear();
        MoveComp->SetMovementMode(MOVE_Falling);
    }

    const float Damage = StatComponent
        ? StatComponent->GetStat().AttackDamage * E_DamageMultiplier
        : 0.0f;

    if (Damage > 0.0f)
    {
        UGameplayStatics::ApplyDamage(
            Target,
            Damage,
            GetController(),
            this,
            ULOL_DamagePhysical::StaticClass()
        );
    }

    Target->LaunchCharacter(FVector(0.f, 0.f, 500.f), true, true);

    ResetE();
}
void AChampion_Blitz::OnBasicAttackHit(ACharacter* Target)
{
    Super::OnBasicAttackHit(Target);

    UE_LOG(LogTemp, Warning, TEXT("Blitz OnBasicAttackHit Called. EActive=%d Target=%s"),
        bIsEActive,
        Target ? *Target->GetName() : TEXT("NULL"));

    if (!HasAuthority()) return;
    if (!bIsEActive) return;
    if (!Target) return;

    OnAttackHitWithE(Target);
}
void AChampion_Blitz::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    Server_Skill_R();
}

bool AChampion_Blitz::Server_Skill_R_Validate() { return true; }

void AChampion_Blitz::Server_Skill_R_Implementation()
{
    if (!SkillComponent || !GetWorld()) return;

    FSkillData& RData = SkillComponent->GetR_Data();

    int32 SkillLevelIdx = 0;

    if (!RData.ManaCost.IsValidIndex(SkillLevelIdx)) return;
    if (!RData.Cooldown.IsValidIndex(SkillLevelIdx)) return;

    if (!SkillComponent->TryCastSkill("R", 1)) return;

    float Radius = RData.Range.IsValidIndex(SkillLevelIdx) ? RData.Range[SkillLevelIdx] : 600.0f;
    if (Radius <= 0.f) Radius = 600.0f;
    float SilenceDuration = RData.Duration.IsValidIndex(SkillLevelIdx) ? RData.Duration[SkillLevelIdx] : 0.5f;
    float BaseDamage = RData.BaseDamage.IsValidIndex(SkillLevelIdx) ? RData.BaseDamage[SkillLevelIdx] : 0.0f;
    float SkillDamage = BaseDamage + StatComponent->GetStat().AbilityPower * R_APRatio;

    if (ChampionResource.RMontage.IsValidIndex(AM_SKIll_R_IDX))
    {
        Multicast_PlayMontage(ChampionResource.RMontage[AM_SKIll_R_IDX], 1.0f);
    }

    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    const bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (!bHit) return;

    TSet<AActor*> HitTargets;

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!IsValidBlitzSkillTarget(Target) || HitTargets.Contains(Target)) continue;

        HitTargets.Add(Target);

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );

        if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
        {
            TargetChampion->Multicast_ApplySilence(SilenceDuration);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[Blitz R] Radius=%.1f Damage=%.1f Hit=%d"),
        Radius, SkillDamage, HitTargets.Num());
}
