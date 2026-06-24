#include "Champion/Champion_Ezreal.h"

#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/Champion_SkillComponent.h"
#include "BaseChampion.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Animation/AnimMontage.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AChampion_Ezreal::AChampion_Ezreal()
{
    ChampionName = TEXT("Ezreal");
    SetChampionData(ChampionName);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> QProjectileMeshAsset(
        TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_q_missile.ezreal_q_missile"));
    if (QProjectileMeshAsset.Succeeded())
    {
        QProjectileMesh = QProjectileMeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> WProjectileMeshAsset(
        TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_w.ezreal_w"));
    if (WProjectileMeshAsset.Succeeded())
    {
        WProjectileMesh = WProjectileMeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RProjectileMeshAsset(
        TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_r.ezreal_r"));
    if (RProjectileMeshAsset.Succeeded())
    {
        RProjectileMesh = RProjectileMeshAsset.Object;
    }

}

void AChampion_Ezreal::Skill_Q()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_Q(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_W()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_W(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_E(Hit.ImpactPoint);
    }
}

void AChampion_Ezreal::Skill_R()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        Server_Skill_R(Hit.ImpactPoint);
    }
}

bool AChampion_Ezreal::Server_Skill_Q_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_W_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_E_Validate(FVector TargetLocation) { return true; }
bool AChampion_Ezreal::Server_Skill_R_Validate(FVector TargetLocation) { return true; }

void AChampion_Ezreal::Server_Skill_Q_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;

    FSkillData& QData = SkillComponent->GetQ_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    const float Range = QData.Range.IsValidIndex(SkillLevelIdx)
        ? QData.Range[SkillLevelIdx]
        : 1200.0f;
    const float BaseDamage = QData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? QData.BaseDamage[SkillLevelIdx]
        : 0.0f;
    const float SkillDamage =
        BaseDamage + StatComponent->GetStat().AttackDamage * 1.0f;
    const FVector ProjectileStart =
        GetActorLocation() + LookRotation.Vector() * 120.0f + FVector(0.0f, 0.0f, 80.0f);
    const FVector ProjectileEnd =
        ProjectileStart + LookRotation.Vector() * Range;
    const float TravelTime =
        Range / FMath::Max(QProjectileSpeed, KINDA_SMALL_NUMBER);
    Multicast_SpawnEzrealProjectile(
        0,
        ProjectileStart,
        ProjectileEnd,
        TravelTime,
        60.0f,
        SkillDamage,
        nullptr,
        false
    );

    if (ChampionResource.QMontage.IsValidIndex(AM_SKIll_Q_IDX) &&
        ChampionResource.QMontage[AM_SKIll_Q_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.QMontage[AM_SKIll_Q_IDX],
            1.0f,
            LookRotation
        );
    }

}

void AChampion_Ezreal::Server_Skill_W_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("W", 1)) return;

    FSkillData& WData = SkillComponent->GetW_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    const float Range = WData.Range.IsValidIndex(SkillLevelIdx)
        ? WData.Range[SkillLevelIdx]
        : 1200.0f;
    const float BaseDamage = WData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? WData.BaseDamage[SkillLevelIdx]
        : 0.0f;
    const FVector ProjectileStart =
        GetActorLocation() + LookRotation.Vector() * 120.0f + FVector(0.0f, 0.0f, 80.0f);
    const FVector ProjectileEnd =
        ProjectileStart + LookRotation.Vector() * Range;
    const float TravelTime =
        Range / FMath::Max(WProjectileSpeed, KINDA_SMALL_NUMBER);
    Multicast_SpawnEzrealProjectile(
        1,
        ProjectileStart,
        ProjectileEnd,
        TravelTime,
        70.0f,
        BaseDamage,
        ULOL_DamageMagic::StaticClass(),
        false
    );

    if (ChampionResource.WMontage.IsValidIndex(AM_SKIll_W_IDX) &&
        ChampionResource.WMontage[AM_SKIll_W_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.WMontage[AM_SKIll_W_IDX],
            1.0f,
            LookRotation
        );
    }

}

void AChampion_Ezreal::Server_Skill_E_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("E", 1)) return;

    FSkillData& EData = SkillComponent->GetE_Data();
    const int32 SkillLevelIdx = 0;

    const float Range = EData.Range.IsValidIndex(SkillLevelIdx)
        ? EData.Range[SkillLevelIdx]
        : 475.0f;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    Direction = Direction.GetSafeNormal();

    FVector BlinkLocation = GetActorLocation() + Direction * Range;
    BlinkLocation.Z = GetActorLocation().Z;

    if (FVector::Dist2D(GetActorLocation(), TargetLocation) < Range)
    {
        BlinkLocation = TargetLocation;
        BlinkLocation.Z = GetActorLocation().Z;
    }

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    if (ChampionResource.EMontage.IsValidIndex(AM_SKIll_E_IDX) &&
        ChampionResource.EMontage[AM_SKIll_E_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.EMontage[AM_SKIll_E_IDX],
            1.0f,
            LookRotation
        );
    }

    SetActorLocation(BlinkLocation, true);

    const float DamageRadius = 650.0f;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(DamageRadius),
        Params
    );

    ABaseChampion* NearestTarget = nullptr;
    float NearestDistSq = TNumericLimits<float>::Max();

    for (const FHitResult& Hit : Hits)
    {
        ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
        if (!TargetChampion || TargetChampion == this) continue;

        const float DistSq = FVector::DistSquared(GetActorLocation(), TargetChampion->GetActorLocation());
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestTarget = TargetChampion;
        }
    }

    if (NearestTarget)
    {
        const float BaseDamage = EData.BaseDamage.IsValidIndex(SkillLevelIdx)
            ? EData.BaseDamage[SkillLevelIdx]
            : 0.0f;

        const float ADRatio = EData.SecondaryValue.IsValidIndex(SkillLevelIdx)
            ? EData.SecondaryValue[SkillLevelIdx]
            : 0.75f;

        const float SkillDamage =
            BaseDamage + StatComponent->GetStat().AttackDamage * ADRatio;

        UGameplayStatics::ApplyDamage(
            NearestTarget,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}

void AChampion_Ezreal::Server_Skill_R_Implementation(FVector TargetLocation)
{
    if (!SkillComponent || !StatComponent) return;
    if (!SkillComponent->TryCastSkill("R", 1)) return;

    FSkillData& RData = SkillComponent->GetR_Data();
    const int32 SkillLevelIdx = 0;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;

    FRotator LookRotation = Direction.Rotation();
    SetActorRotation(LookRotation);

    const float Range = RData.Range.IsValidIndex(SkillLevelIdx)
        ? RData.Range[SkillLevelIdx]
        : 2500.0f;
    const float BaseDamage = RData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? RData.BaseDamage[SkillLevelIdx]
        : 0.0f;
    const float SkillDamage =
        BaseDamage + StatComponent->GetStat().AttackDamage * 0.9f;
    const FVector ProjectileStart =
        GetActorLocation() + LookRotation.Vector() * 140.0f + FVector(0.0f, 0.0f, 90.0f);
    const FVector ProjectileEnd =
        ProjectileStart + LookRotation.Vector() * Range;
    const float TravelTime =
        Range / FMath::Max(RProjectileSpeed, KINDA_SMALL_NUMBER);
    Multicast_SpawnEzrealProjectile(
        2,
        ProjectileStart,
        ProjectileEnd,
        TravelTime,
        280.0f,
        SkillDamage,
        ULOL_DamageMagic::StaticClass(),
        true
    );

    if (ChampionResource.RMontage.IsValidIndex(AM_SKIll_R_IDX) &&
        ChampionResource.RMontage[AM_SKIll_R_IDX])
    {
        Multicast_PlayEzrealSkillMontage(
            ChampionResource.RMontage[AM_SKIll_R_IDX],
            1.0f,
            LookRotation
        );
    }

}

void AChampion_Ezreal::Multicast_PlayEzrealSkillMontage_Implementation(
    UAnimMontage* Montage,
    float PlayRate,
    FRotator NewRotation
)
{
    SetActorRotation(NewRotation);

    if (Montage)
    {
        PlayAnimMontage(Montage, PlayRate);
    }
}

void AChampion_Ezreal::Multicast_SpawnEzrealProjectile_Implementation(
    uint8 ProjectileType,
    FVector StartLocation,
    FVector EndLocation,
    float TravelTime,
    float CollisionRadius,
    float Damage,
    TSubclassOf<UDamageType> DamageType,
    bool bHitMultiple)
{
    SpawnEzrealProjectileVisual(
        ProjectileType,
        StartLocation,
        EndLocation,
        TravelTime,
        CollisionRadius,
        Damage,
        DamageType,
        bHitMultiple
    );
}

void AChampion_Ezreal::OnEzrealProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OverlappedComponent || !OtherActor)
    {
        return;
    }

    AActor* ProjectileActor = OverlappedComponent->GetOwner();
    if (!ProjectileActor || OtherActor == this || OtherActor == ProjectileActor)
    {
        return;
    }

    FEzrealProjectileDamageData* ProjectileData =
        ActiveProjectiles.Find(ProjectileActor);
    if (!ProjectileData)
    {
        return;
    }

    ULOL_StateComponent* TargetState = OtherActor->FindComponentByClass<ULOL_StateComponent>();
    if (!TargetState || !IsEnemyActor(OtherActor))
    {
        return;
    }

    const TWeakObjectPtr<AActor> TargetKey(OtherActor);
    if (ProjectileData->DamagedTargets.Contains(TargetKey))
    {
        return;
    }

    ProjectileData->DamagedTargets.Add(TargetKey);

    const bool bDestroyOnHit = !ProjectileData->bHitMultiple;

    if (HasAuthority())
    {
        UGameplayStatics::ApplyDamage(
            OtherActor,
            ProjectileData->Damage,
            GetController(),
            this,
            ProjectileData->DamageType
        );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Ezreal projectile hit. Target=%s Damage=%f MultiHit=%d DestroyOnHit=%d"),
            *OtherActor->GetName(),
            ProjectileData->Damage,
            ProjectileData->bHitMultiple,
            bDestroyOnHit
        );
    }

    if (bDestroyOnHit)
    {
        ActiveProjectiles.Remove(ProjectileActor);
        ProjectileActor->Destroy();
    }
}

UStaticMesh* AChampion_Ezreal::GetEzrealProjectileMesh(uint8 ProjectileType)
{
    switch (ProjectileType)
    {
    case 0:
        if (!QProjectileMesh)
        {
            QProjectileMesh = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_q_missile.ezreal_q_missile")
            );
        }
        return QProjectileMesh.Get();
    case 1:
        if (!WProjectileMesh)
        {
            WProjectileMesh = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_w.ezreal_w")
            );
        }
        return WProjectileMesh.Get();
    case 2:
        if (!RProjectileMesh)
        {
            RProjectileMesh = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Game/Level/ezreal/ezreal_tex/ezreal_r.ezreal_r")
            );
        }
        return RProjectileMesh.Get();
    default:
        return nullptr;
    }
}

void AChampion_Ezreal::SpawnEzrealProjectileVisual(
    uint8 ProjectileType,
    FVector StartLocation,
    FVector EndLocation,
    float TravelTime,
    float CollisionRadius,
    float Damage,
    TSubclassOf<UDamageType> DamageType,
    bool bHitMultiple)
{
    if (!GetWorld())
    {
        return;
    }

    UStaticMesh* ProjectileMeshAsset = GetEzrealProjectileMesh(ProjectileType);
    if (!ProjectileMeshAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Ezreal projectile mesh missing. ProjectileType=%d"), ProjectileType);
        return;
    }

    const FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return;
    }

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
    if (!ProjectileActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Ezreal projectile actor spawn failed. ProjectileType=%d"), ProjectileType);
        return;
    }
    ProjectileActor->SetActorHiddenInGame(false);

    USphereComponent* CollisionComponent =
        NewObject<USphereComponent>(
            ProjectileActor,
            TEXT("EzrealProjectileCollision")
        );
    if (!CollisionComponent)
    {
        ProjectileActor->Destroy();
        return;
    }

    ProjectileActor->AddInstanceComponent(CollisionComponent);
    ProjectileActor->SetRootComponent(CollisionComponent);
    CollisionComponent->SetMobility(EComponentMobility::Movable);
    CollisionComponent->InitSphereRadius(CollisionRadius);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionComponent->SetGenerateOverlapEvents(true);
    CollisionComponent->RegisterComponent();

    CollisionComponent->OnComponentBeginOverlap.AddDynamic(
        this,
        &AChampion_Ezreal::OnEzrealProjectileOverlap
    );

    UStaticMeshComponent* MeshComponent =
        NewObject<UStaticMeshComponent>(
            ProjectileActor,
            TEXT("EzrealProjectileMesh")
        );
    if (!MeshComponent)
    {
        ProjectileActor->Destroy();
        return;
    }

    ProjectileActor->AddInstanceComponent(MeshComponent);
    MeshComponent->SetupAttachment(CollisionComponent);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(ProjectileMeshAsset);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
    MeshComponent->RegisterComponent();

    ProjectileActor->SetActorLocationAndRotation(
        StartLocation,
        Direction.Rotation(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );
    const float MeshRadius = ProjectileMeshAsset->GetBounds().SphereRadius;
    float VisualRadius = QProjectileVisualRadius;
    FVector VisualScale3D = QProjectileVisualScale3D;
    FRotator RotationOffset = QProjectileRotationOffset;
    if (ProjectileType == 1)
    {
        VisualRadius = WProjectileVisualRadius;
        VisualScale3D = WProjectileVisualScale3D;
        RotationOffset = WProjectileRotationOffset;
    }
    else if (ProjectileType == 2)
    {
        VisualRadius = RProjectileVisualRadius;
        VisualScale3D = RProjectileVisualScale3D;
        RotationOffset = RProjectileRotationOffset;
    }
    MeshComponent->SetRelativeRotation(RotationOffset);

    const float Scale = MeshRadius > KINDA_SMALL_NUMBER
        ? VisualRadius / MeshRadius
        : 1.0f;
    MeshComponent->SetWorldScale3D(
        FVector(Scale * ProjectileVisualScale) * VisualScale3D
    );
    MeshComponent->SetVisibility(true, true);
    MeshComponent->SetHiddenInGame(false, true);

    UProjectileMovementComponent* ProjectileMovement =
        NewObject<UProjectileMovementComponent>(
            ProjectileActor,
            TEXT("EzrealProjectileMovement")
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
    ProjectileMovement->SetUpdatedComponent(CollisionComponent);
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->InitialSpeed = VisualSpeed;
    ProjectileMovement->MaxSpeed = VisualSpeed;
    ProjectileMovement->Velocity = Direction * VisualSpeed;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bRotationFollowsVelocity = false;
    ProjectileMovement->RegisterComponent();
    ProjectileMovement->Activate(true);

    ProjectileActor->SetLifeSpan(FMath::Max(TravelTime + 0.1f, 0.2f));

    FEzrealProjectileDamageData ProjectileDamageData;
    ProjectileDamageData.Damage = Damage;
    ProjectileDamageData.DamageType = DamageType;
    ProjectileDamageData.bHitMultiple = bHitMultiple;
    ActiveProjectiles.Add(ProjectileActor, ProjectileDamageData);

    FTimerHandle CleanupTimerHandle;
    GetWorldTimerManager().SetTimer(
        CleanupTimerHandle,
        FTimerDelegate::CreateLambda(
            [this, ProjectileActor]()
            {
                ActiveProjectiles.Remove(ProjectileActor);
            }
        ),
        FMath::Max(TravelTime + 0.2f, 0.3f),
        false
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Ezreal projectile spawned. Type=%d Mesh=%s Start=%s End=%s Scale=%f TravelTime=%f"),
        ProjectileType,
        *ProjectileMeshAsset->GetName(),
        *StartLocation.ToString(),
        *EndLocation.ToString(),
        Scale,
        TravelTime
    );
}

void AChampion_Ezreal::ApplyEzrealLineSkill(
    FVector TargetLocation,
    FSkillData& SkillData,
    int32 SkillLevelIdx,
    float Radius,
    TSubclassOf<UDamageType> DamageType,
    float ADRatio,
    bool bHitMultiple
)
{
    if (!StatComponent || !GetWorld()) return;

    FVector Direction = TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;

    if (Direction.IsNearlyZero()) return;

    Direction = Direction.GetSafeNormal();

    const float Range = SkillData.Range.IsValidIndex(SkillLevelIdx)
        ? SkillData.Range[SkillLevelIdx]
        : 1200.0f;

    const float BaseDamage = SkillData.BaseDamage.IsValidIndex(SkillLevelIdx)
        ? SkillData.BaseDamage[SkillLevelIdx]
        : 0.0f;

    const float SkillDamage =
        BaseDamage + StatComponent->GetStat().AttackDamage * ADRatio;

    FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    FVector End = Start + Direction * Range;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (bHitMultiple)
    {
        TArray<FHitResult> Hits;

        const bool bHit = GetWorld()->SweepMultiByChannel(
            Hits,
            Start,
            End,
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(Radius),
            Params
        );

        if (!bHit) return;

        TSet<ABaseChampion*> DamagedTargets;

        for (const FHitResult& Hit : Hits)
        {
            ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
            if (!TargetChampion || TargetChampion == this) continue;
            if (DamagedTargets.Contains(TargetChampion)) continue;

            DamagedTargets.Add(TargetChampion);

            UGameplayStatics::ApplyDamage(
                TargetChampion,
                SkillDamage,
                GetController(),
                this,
                DamageType
            );
        }

        return;
    }

    FHitResult Hit;

    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(Radius),
        Params
    );

    if (!bHit) return;

    ABaseChampion* TargetChampion = Cast<ABaseChampion>(Hit.GetActor());
    if (!TargetChampion || TargetChampion == this) return;

    UGameplayStatics::ApplyDamage(
        TargetChampion,
        SkillDamage,
        GetController(),
        this,
        DamageType
    );
}
