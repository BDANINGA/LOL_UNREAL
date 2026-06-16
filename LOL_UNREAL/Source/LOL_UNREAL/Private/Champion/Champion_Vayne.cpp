#include "Champion/Champion_Vayne.h"
#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"

#include "Components/CapsuleComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"     // 2026 05 01 
#include "GameFramework/DamageType.h"   // 2026 05 01 (UDamageType용)

AChampion_Vayne::AChampion_Vayne()
{
    ChampionName = TEXT("Vayne");
    SetChampionData(ChampionName);
    StateComponent->AddStatusTag(LOLTags::Champion_Ranged);
}

void AChampion_Vayne::Skill_Q()
{
    if (!IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // 2026 05 01 수정
    FHitResult Hit;
    if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return;
    Server_Skill_Q(Hit.Location);
}

bool AChampion_Vayne::Server_Skill_Q_Validate(FVector QLocation) { return true; }

void AChampion_Vayne::Multicast_PlayQMontage_Implementation()
{
    if (ChampionResource.QMontage[AM_SKIll_Q_IDX])
    {
        PlayAnimMontage(ChampionResource.QMontage[AM_SKIll_Q_IDX], 1.0f);
    }
    else
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("NO (Ultimate) Used!"));
}

void AChampion_Vayne::Server_Skill_Q_Implementation(FVector QLocation)
{
    if (bIsDashing) return;
    if (!SkillComponent->TryCastSkill("Q", 1)) return;
    FVector Start = GetActorLocation();

    // 1. 수평 방향 계산 (Z값 무시) ------- 2026 05 01 수정
    FVector Direction = (QLocation - Start).GetSafeNormal2D();  // Z=0 + Normalize 한 번에
    if (Direction.IsNearlyZero()) return;

    // 2026 05 04 추가
    Multicast_PlayQMontage();

    DashStart = Start;

    // 2. 임시 목적지 설정 (평면 기준 300 유닛 앞)
    FVector TempTarget = Start + Direction * 300.0f;

    // 3. 해당 지점의 지형 높이 찾기 (LineTrace)
    FHitResult FloorHit;
    float TraceRange = 500.0f; // 위아래로 탐색할 범위
    FVector TraceStart = TempTarget + FVector(0, 0, TraceRange);
    FVector TraceEnd = TempTarget - FVector(0, 0, TraceRange);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 본인은 무시

    if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        // 바닥을 찾았다면: 바닥 위치 + 캐릭터 절반 높이(캡슐)만큼 위로 설정
        float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        DashTarget = FloorHit.Location + FVector(0, 0, HalfHeight);
    }
    else
    {
        // 바닥을 못 찾았다면 (절벽 등): 그냥 평면 위치 유지
        DashTarget = TempTarget;
    }

    DashElapsed = 0.0f;
    GetCharacterMovement()->SetMovementMode(MOVE_None);
    bIsDashing = true;

    // ★ 추가 — 다음 평타 강화 플래그 ON
    bQEmpowered = true;

    UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 다음 평타 강화!"));

    // 만료 타이머 (6초 안에 평타 안 치면 사라짐)
    GetWorld()->GetTimerManager().SetTimer(
        Q_EmpoweredTimerHandle,
        this,
        &AChampion_Vayne::EndQEmpower,
        Q_EmpowerDuration,
        false
    );
}

void AChampion_Vayne::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDashing)
    {
        DashElapsed += DeltaTime;
        float Alpha = FMath::Clamp(DashElapsed / DashTime, 0.0f, 1.0f);
        FVector NewLocation = FMath::Lerp(DashStart, DashTarget, Alpha);

        // 수정: 두 번째 인자를 false로 변경하여 충돌 검사(Sweep)를 끕니다.
        // 이미 목적지가 지형 위로 계산되었으므로, 경로상의 지형 마찰에 멈추지 않게 합니다.
        SetActorLocation(NewLocation, false);

        if (Alpha >= 1.0f)
        {
            bIsDashing = false;
            // 이동 완료 후 다시 걷기 모드로 복구
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }

    if (GEngine)
    {
        const float Speed = GetCharacterMovement()->MaxWalkSpeed;
        const float AD = StatComponent->GetStat().AttackDamage;

        GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green,
            FString::Printf(TEXT("MaxWalkSpeed: 90.0f"), Speed));
        GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
            FString::Printf(TEXT("AttackDamage: 35.0f"), AD));
    }

    if (IsLocallyControlled() && bIsChasingForE)
    {
        UpdateEChaseToCast();
    }
}

void AChampion_Vayne::Skill_W()
{
    UE_LOG(LogTemp, Log, TEXT("[Vayne W] 은빛 화살은 패시브입니다."));
}

void AChampion_Vayne::EndQEmpower()
{
    if (bQEmpowered)
    {
        bQEmpowered = false;
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 강화 평타 만료 (사용 안 함)"));
    }
}

void AChampion_Vayne::OnBasicAttackHit(ACharacter* Target)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!IsValid(Target) || Target == this)
    {
        return;
    }

    if (bQEmpowered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 강화 평타 발동!"));

        // ★ 1단계 — Target 확인
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 1단계: Target = %s"),
            IsValid(Target) ? *Target->GetName() : TEXT("nullptr"));

        // ★ 2단계 — SkillComponent 확인
        if (!SkillComponent)
        {
            UE_LOG(LogTemp, Error, TEXT("[Vayne Q] SkillComponent가 nullptr!"));
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 2단계: SkillComponent OK"));

        // ★ 3단계 — Q 데이터 확인
        const FSkillData& QData = SkillComponent->GetQ_Data();
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 3단계: BaseDamage 크기 = %d"),
            QData.BaseDamage.Num());

        if (QData.BaseDamage.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("[Vayne Q] BaseDamage 배열이 비어 있음!"));
            bQEmpowered = false;
            return;
        }

        // ★ 4단계 — StatComponent 확인
        if (!StatComponent)
        {
            UE_LOG(LogTemp, Error, TEXT("[Vayne Q] StatComponent가 nullptr!"));
            return;
        }

        // ★ 5단계 — 데미지 계산
        float SkillDamage = QData.BaseDamage[0] +
            StatComponent->GetStat().AttackDamage * 0.5f;

        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 5단계: SkillDamage = %.1f"), SkillDamage);

        // ★ 6단계 — ApplyDamage 호출
        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 6단계: ApplyDamage 호출 직전"));

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            UDamageType::StaticClass()
        );

        UE_LOG(LogTemp, Warning, TEXT("[Vayne Q] 7단계: ApplyDamage 호출 완료, 적 HP 줄었어야 함"));

        bQEmpowered = false;
        GetWorldTimerManager().ClearTimer(Q_EmpoweredTimerHandle);
    }

    TWeakObjectPtr<ACharacter> Key(Target);

    // 1. 해당 타겟의 스택 +1
    int32& Stack = SilverBoltsStack.FindOrAdd(Key);
    Stack++;

    UE_LOG(LogTemp, Log, TEXT("[Vayne W] %s 스택: %d / %d"),
        *Target->GetName(), Stack, BoltsThreshold);

    // 2. 임계값 도달 → 발동
    if (Stack >= BoltsThreshold)
    {
        TriggerSilverBolts(Target);
        Stack = 0;  // 리셋

        // 발동했으니 만료 타이머도 정리
        if (FTimerHandle* ExistingTimer = SilverBoltsTimers.Find(Key))
        {
            GetWorld()->GetTimerManager().ClearTimer(*ExistingTimer);
            SilverBoltsTimers.Remove(Key);
        }
        return;
    }

    // 3. 아직 임계값 미달 → 만료 타이머 갱신
    //    (4초간 같은 적 안 때리면 스택 사라짐)
    FTimerHandle& TimerHandle = SilverBoltsTimers.FindOrAdd(Key);
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle);  // 기존 타이머 취소

    GetWorld()->GetTimerManager().SetTimer(TimerHandle,
        FTimerDelegate::CreateLambda([this, Key]()
            {
                if (!IsValid(this)) return;
                SilverBoltsStack.Remove(Key);
                SilverBoltsTimers.Remove(Key);

                UE_LOG(LogTemp, Log, TEXT("[Vayne W] 스택 만료"));
            }),
        BoltsStackDuration, false);
}

void AChampion_Vayne::TriggerSilverBolts(ACharacter* Target)
{
    if (!IsValid(Target)) return;

    float SkillDamage = SkillComponent->GetE_Data().BaseDamage[0] + StatComponent->GetStat().AttackDamage * 0.5f + BoltsBonusDamage;

    // 1. 추가 피해 적용 (서버에서)
    UGameplayStatics::ApplyDamage(
        Target,
        SkillDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    UE_LOG(LogTemp, Warning, TEXT("[Vayne W] 은빛 화살 발동! %s에게 %.0f 고정 피해"),
        *Target->GetName(), BoltsBonusDamage);
}

void AChampion_Vayne::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (bIsStunned || bIsKnockedBack) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController()) return;

    FHitResult Hit;
    if (!PC->GetHitResultUnderCursor(ECC_Pawn, false, Hit)) return;

    ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
    if (!IsValid(Target) || Target == this) return;

    const float Range = GetESkillRange();
    const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

    if (Distance <= Range)
    {
        bIsChasingForE = false;
        ReservedETarget = nullptr;

        FVector Direction = Target->GetActorLocation() - GetActorLocation();
        Direction.Z = 0.0f;

        if (!Direction.IsNearlyZero())
        {
            SetActorRotation(Direction.Rotation());
        }

        Server_ExecuteCondemn(Target);
        return;
    }

    ReservedETarget = Target;
    bIsChasingForE = true;

    if (ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>())
    {
        StateComp->AddStatusTag(LOLTags::State_Moving);
        StateComp->RemoveStatusTag(LOLTags::State_Attacking);
    }
}

void AChampion_Vayne::UpdateEChaseToCast()
{
    if (bIsStunned || bIsKnockedBack || !IsValid(ReservedETarget))
    {
        bIsChasingForE = false;
        ReservedETarget = nullptr;
        return;
    }

    ULOL_StateComponent* StateComp = FindComponentByClass<ULOL_StateComponent>();
    ULOL_MoveComponent* MoveComp = FindComponentByClass<ULOL_MoveComponent>();
    if (!StateComp || !MoveComp) return;

    const float Range = GetESkillRange();
    const float Distance = FVector::Dist(GetActorLocation(), ReservedETarget->GetActorLocation());

    if (Distance <= Range)
    {
        ACharacter* Target = ReservedETarget;

        bIsChasingForE = false;
        ReservedETarget = nullptr;

        MoveComp->StopMovement();
        StateComp->RemoveStatusTag(LOLTags::State_Moving);

        FVector Direction = Target->GetActorLocation() - GetActorLocation();
        Direction.Z = 0.0f;

        if (!Direction.IsNearlyZero())
        {
            SetActorRotation(Direction.Rotation());
        }

        Server_ExecuteCondemn(Target);
        return;
    }

    StateComp->AddStatusTag(LOLTags::State_Moving);
    StateComp->RemoveStatusTag(LOLTags::State_Attacking);

    MoveComp->TargetLocation = ReservedETarget->GetActorLocation();

    FVector Direction = MoveComp->TargetLocation - GetActorLocation();
    Direction.Z = 0.0f;

    if (!Direction.IsNearlyZero())
    {
        AddMovementInput(Direction.GetSafeNormal(), 1.0f);
    }
}

float AChampion_Vayne::GetESkillRange()
{
    if (!SkillComponent) return 500.0f;

    FSkillData& EData = SkillComponent->GetE_Data();
    const int32 SkillLevelIdx = 0;

    return EData.Range.IsValidIndex(SkillLevelIdx)
        ? EData.Range[SkillLevelIdx]
        : 500.0f;
}

void AChampion_Vayne::Multicast_PlayEMontage_Implementation()
{
    if (ChampionResource.EMontage[AM_SKIll_E_IDX])
    {
        PlayAnimMontage(ChampionResource.EMontage[AM_SKIll_E_IDX], 2.0f);
    }
}

void AChampion_Vayne::Multicast_SetCondemnRotation_Implementation(FRotator NewRotation)
{
    SetActorRotation(NewRotation);
}

// 서버 RPC 검증
bool AChampion_Vayne::Server_ExecuteCondemn_Validate(ACharacter* Target)
{
    if (!IsValid(Target)) return false; 
    return true;
}

// 서버 RPC 실제 실행 (AChampion_Vayne:: 를 반드시 붙여야 함)
void AChampion_Vayne::Server_ExecuteCondemn_Implementation(ACharacter* Target)
{
    if (!SkillComponent || !StatComponent) return;
    if (!IsValid(Target) || !Target->GetCharacterMovement()) return;

    const float Range = GetESkillRange();
    if (FVector::Dist(GetActorLocation(), Target->GetActorLocation()) > Range + 50.0f)
    {
        return;
    }

    if (!SkillComponent->TryCastSkill("E", 1)) return;

    // ★ 회전 멀티캐스트 (모든 클라에서 베인이 적을 향해 돎)
    FVector RotDirection = Target->GetActorLocation() - GetActorLocation();
    RotDirection.Z = 0.0f;
    if (!RotDirection.IsNearlyZero())
    {
        FRotator LookRotation = RotDirection.Rotation();
        LookRotation.Pitch = 0.0f;
        LookRotation.Roll = 0.0f;
        Multicast_SetCondemnRotation(LookRotation);
    }

    // ★ 모션 멀티캐스트 (모든 클라에서 E 모션 재생) ← 이게 핵심!
    Multicast_PlayEMontage();

    // 밀쳐내기
    FVector MyLoc = GetActorLocation();
    FVector TargetLoc = Target->GetActorLocation();
    FVector PushDir = (TargetLoc - MyLoc).GetSafeNormal2D();
    FVector LaunchVelocity = (PushDir * PushDistance) / PushTime;

    if (ABaseChampion* TargetChampion = Cast<ABaseChampion>(Target))
    {
        TargetChampion->StartKnockbackWithWallCheck(LaunchVelocity, PushTime, WallStunDuration);
    }
    else
    {
        Target->LaunchCharacter(LaunchVelocity, true, true);
    }

    float SkillDamage = SkillComponent->GetE_Data().BaseDamage[0] + StatComponent->GetStat().AttackDamage * 0.5f;

    UGameplayStatics::ApplyDamage(
        Target,
        SkillDamage,
        this->GetController(),
        this,
        ULOL_DamageMagic::StaticClass()
    );
}

void AChampion_Vayne::Skill_R()
{
    
    // 로컬 컨트롤러에서 서버로 요청
    if (IsLocallyControlled())
    {
        Server_Skill_R();
    }
}

bool AChampion_Vayne::Server_Skill_R_Validate() { return true; }

void AChampion_Vayne::Server_Skill_R_Implementation()
{
    // 1. 상태 활성화 및 스태츠 버프 (서버)
    AM_Atk_Idx = 1;
    StateComponent->AddStatusTag(LOLTags::Skill_R);
    // 실제 데미지 계산 로직이 StatComponent 등에 있다면 해당 수치를 가산하세요.
    // 예: StatComponent->AddAttackDamage(R_BonusAD);

    // 2. 타이머 설정 (종료 예약)
    GetWorldTimerManager().ClearTimer(R_TimerHandle);
    GetWorldTimerManager().SetTimer(R_TimerHandle, this, &AChampion_Vayne::End_Skill_R, R_Duration, false);

    // 3. 연출 실행 (멀티캐스트)
    Multicast_PlayRMontage();

    UE_LOG(LogTemp, Log, TEXT("[Vayne] 결전의 시간 발동!"));
}

void AChampion_Vayne::Multicast_PlayRMontage_Implementation()
{
    if (ChampionResource.RMontage[AM_SKIll_R_IDX])
    {
        PlayAnimMontage(ChampionResource.RMontage[AM_SKIll_R_IDX]);
    }
}

void AChampion_Vayne::End_Skill_R()
{
    StateComponent->RemoveStatusTag(LOLTags::Skill_R);
    AM_Atk_Idx = 0;
    // StatComponent->AddAttackDamage(-R_BonusAD); // 버프 회수

    UE_LOG(LogTemp, Log, TEXT("[Vayne] 결전의 시간 종료"));
}

void AChampion_Vayne::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AChampion_Vayne, bQEmpowered);   // ★ 추가
}
