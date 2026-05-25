#include "Champion/Champion_Alistar.h"

#include "Component/Champion_SkillComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "UObject/ConstructorHelpers.h"

AChampion_Alistar::AChampion_Alistar()
{
    ChampionName = TEXT("Alistar");
    SetChampionData(ChampionName);
}

void AChampion_Alistar::Multicast_PlayQMontage_Implementation()
{
    if (ChampionResource.QMontage)
    {
        PlayAnimMontage(ChampionResource.QMontage, 2.0f);
    }
}

void AChampion_Alistar::Multicast_PlayWMontage_Implementation()
{
    if (ChampionResource.WMontage)
    {
        PlayAnimMontage(ChampionResource.WMontage, 2.0f);
    }
}

void AChampion_Alistar::Multicast_PlayRMontage_Implementation()
{
    if (ChampionResource.RMontage)
    {
        PlayAnimMontage(ChampionResource.RMontage, 1.0f);
    }
}

void AChampion_Alistar::EndQCast()
{
    // 이동 복구
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    bCanAttack = true;
}

void AChampion_Alistar::Skill_Q()
{
    if (!IsLocallyControlled()) return;
   
    // 본인 시전 잠금 (본인 클라 즉시 반응)
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();

    // 본인 시전 락 풀 타이머
    GetWorld()->GetTimerManager().SetTimer(
        Q_CastTimerHandle,
        this,
        &AChampion_Alistar::EndQCast,
        Q_CastTime,
        false
    );

    // ★ 진짜 로직은 서버에서
    Server_Skill_Q();
}

bool AChampion_Alistar::Server_Skill_Q_Validate() { return true; }

void AChampion_Alistar::Server_Skill_Q_Implementation()
{
    if (!SkillComponent->TryCastSkill(SkillComponent->GetQ_Data(), 1)) return;

    // ★ 시각 효과는 서버에서 멀티캐스트 (모든 클라에 전파)
    Multicast_PlayQMontage();

    // 범위 안 적 검출
    FVector Center = GetActorLocation();
    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(100.0f);

    bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        Center,
        Center,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (!bHit) return;

    // 각 적에게 처리 적용
    for (auto& Hit : Hits)
    {
        ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
        if (!Target || Target == this) continue;

        // 적 모션·이동 정리
        Target->StopAnimMontage();
        UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement();
        if (MoveComp)
        {
            MoveComp->StopMovementImmediately();
            MoveComp->CurrentRootMotion.Clear();
            MoveComp->SetMovementMode(MOVE_Falling);
        }

        // 캡슐 충돌 무시
        Target->MoveIgnoreActorAdd(this);

        // 에어본 발사
        Target->LaunchCharacter(FVector(0, 0, 600.0f), true, true);

        // 스턴 + 에어본 상태 처리
        if (ABaseChampion* Champ = Cast<ABaseChampion>(Target))
        {
            Champ->SetIsKnockedBack(true);
            Champ->ApplyStun(2.0f);

            // 1초 뒤 상태 복구
            FTimerHandle AirTimer;
            GetWorld()->GetTimerManager().SetTimer(AirTimer, [Champ, this]() {
                if (IsValid(Champ))
                {
                    Champ->SetIsKnockedBack(false);
                    if (IsValid(this))
                    {
                        Champ->MoveIgnoreActorRemove(this);
                    }
                }
                }, 1.0f, false);
        }

        // 데미지 적용
        float SkillDamage = SkillComponent->GetQ_Data().BaseDamage[0] +
            StatComponent->GetStat().AttackDamage * 0.8f;

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}

void AChampion_Alistar::Skill_W()
{
    if (!IsLocallyControlled()) return;

    // 마우스 아래 적 검출
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult HitResult;
    if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
    {
        ACharacter* Target = Cast<ACharacter>(HitResult.GetActor());
        if (!Target || Target == this) return;

        // 사거리 체크
        float SkillRange = 550.0f;
        float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
        if (Distance > SkillRange) return;

        // 본인 화면 즉시 회전 (반응성)
        FVector DashDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        FRotator LookRotation = DashDirection.Rotation();
        LookRotation.Pitch = 0.0f;
        LookRotation.Roll = 0.0f;
        SetActorRotation(LookRotation);

        // 서버 RPC
        Server_Skill_W(Target);
    }
}

bool AChampion_Alistar::Server_Skill_W_Validate(ACharacter* Target)
{
    if (!IsValid(Target)) return false;
    return true;
}

void AChampion_Alistar::Server_Skill_W_Implementation(ACharacter* Target)
{
    if (!SkillComponent->TryCastSkill(SkillComponent->GetW_Data(), 1)) return;
    if (!IsValid(Target) || Target == this) return;

    // 멀티캐스트로 모션 전파 (모든 클라)
    Multicast_PlayWMontage();

    // 추적 상태 시작
    CurrentWTarget = Target;
    bIsW_Dashing = true;

    // 회전 (서버 권한, 자동 동기화)
    FVector DashDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    FRotator LookRotation = DashDirection.Rotation();
    LookRotation.Pitch = 0.0f;
    LookRotation.Roll = 0.0f;
    SetActorRotation(LookRotation);

    // 돌진 시작
    float DashSpeed = 1200.0f;
    LaunchCharacter(DashDirection * DashSpeed, true, true);
}

void AChampion_Alistar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!HasAuthority()) return;
    //에어본 상태일때
    if (bIsKnockedBack)
    {
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->StopMovementImmediately();
        }
        return; // 아래의 추적/공격 로직을 실행하지 않음
    }


    if (bIsW_Dashing && CurrentWTarget)
    {
        FVector CurrentLoc = GetActorLocation();
        FVector TargetLoc = CurrentWTarget->GetActorLocation();
        float Distance = FVector::Dist(CurrentLoc, TargetLoc);

        // 타겟 위치로 보간하며 이동 (공격 대상이 움직여도 따라감)
        FVector NewLocation = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, 15.0f);
        SetActorLocation(NewLocation, true);

        // 판정 거리 (100.0f 이내면 충돌로 간주)
        if (Distance < 100.0f)
        {
            ApplyWKnockback(CurrentWTarget);
        }
    }
}

void AChampion_Alistar::ApplyWKnockback(ACharacter* Target)
{
    if (!Target) return;

    // 1. 공통 부모 클래스인 ABaseChampion으로 캐스팅
    ABaseChampion* Enemy = Cast<ABaseChampion>(Target);

    if (Enemy)
    {
        
        // [중요] 2. 상대방의 자율 이동 및 공격 로직 일시 중단
        // BaseChampion의 Tick(CheckAttackRange)에서 이 변수를 체크하여 
        // StopMovementImmediately()가 호출되는 것을 막아야 합니다.
        Enemy->SetIsKnockedBack(true);

        // [중요] 3. 공격 애니메이션 강제 중단 (루트 모션 해제)
        // 평타 중일 때 루트 모션이 위치를 고정하는 것을 방지합니다.
        Enemy->StopAnimMontage();

        UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();
        if (MoveComp)
        {
            // 기존의 모든 속도와 루트 모션 잔여 데이터를 제거
            MoveComp->StopMovementImmediately();
            MoveComp->CurrentRootMotion.Clear();

            // 지면 마찰력을 무시하기 위해 공중 상태로 변경
            MoveComp->SetMovementMode(MOVE_Falling);
        }

        // 4. 넉백 방향 및 속도 계산
        FVector AlistarLoc = GetActorLocation();
        FVector EnemyLoc = Enemy->GetActorLocation();

        // 방향은 수평(XY)으로만 계산
        FVector PushDir = (EnemyLoc - AlistarLoc).GetSafeNormal2D();

        // 0.5초 동안 약 600~700 유닛을 날려보낼 속도 설정
        FVector LaunchVel = PushDir * 1200.0f;
        LaunchVel.Z = 100.0f; // 에어본 효과

        // 5. 상대방 날리기 (기존 속도 무시 옵션 true, true)
        Enemy->LaunchCharacter(LaunchVel, true, true);

        // 6. 0.5초(넉백 시간) 후에 상대방의 상태(bIsKnockedBack)를 정상으로 복구
        // 람다 함수 내에서 IsValid 체크를 통해 캐릭터가 파괴되지 않았는지 확인합니다.
        FTimerHandle RecoveryTimer;
        GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, FTimerDelegate::CreateLambda([Enemy]() {
            if (IsValid(Enemy))
            {
                Enemy->SetIsKnockedBack(false);
            }
            }), 0.5f, false);
    }

    // 7. 알리스타 본인 처리 (타겟이 있던 위치에 멈춤)
    SetActorLocation(Target->GetActorLocation(), true);
    GetCharacterMovement()->StopMovementImmediately();

    // 상태 변수 초기화
    bIsW_Dashing = false;
    CurrentWTarget = nullptr;
}



void AChampion_Alistar::Skill_E()
{
    if (!IsLocallyControlled()) return;
    if (!SkillComponent->TryCastSkill(SkillComponent->GetE_Data(), 1)) return;

    // 마우스 타겟팅 없음 — 자기 주변에 영역 효과
    Server_Skill_E();
}

bool AChampion_Alistar::Server_Skill_E_Validate()
{
    return true;
}

void AChampion_Alistar::Server_Skill_E_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("[Alistar E] 광역 DoT 발동! %d회 데미지"), E_MaxTicks);

    // 틱 카운터 초기화
    E_CurrentTick = 0;

    // ★ 반복 타이머 — 1초마다 ApplyEDamageTick 호출
    GetWorld()->GetTimerManager().SetTimer(
        E_TickTimerHandle,
        this,
        &AChampion_Alistar::ApplyEDamageTick,
        E_TickInterval,
        true   // ★ true = 반복!
    );

    // 즉시 한 번 발동 (선택사항 — 첫 데미지가 1초 후 들어가지 말고 즉시)
    ApplyEDamageTick();
}

void AChampion_Alistar::ApplyEDamageTick()
{
    E_CurrentTick++;

    UE_LOG(LogTemp, Log, TEXT("[Alistar E] %d/%d 틱 발동"),
        E_CurrentTick, E_MaxTicks);

    // 주변 적 검출
    FVector Center = GetActorLocation();
    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(E_Radius);

    bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        Center,
        Center,
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (bHit)
    {
        // 데미지 계산
        float TotalDamage = SkillComponent->GetE_Data().BaseDamage[0] +
            StatComponent->GetStat().AbilityPower * 7.0f;

        float SkillDamage = TotalDamage / E_MaxTicks;

        // 각 적에게 적용
        for (auto& Hit : Hits)
        {
            ACharacter* Target = Cast<ACharacter>(Hit.GetActor());
            if (Target && Target != this)
            {
                UGameplayStatics::ApplyDamage(
                    Target,
                    SkillDamage,
                    GetController(),
                    this,
                    ULOL_DamageMagic::StaticClass()
                );

                UE_LOG(LogTemp, Log, TEXT("[Alistar E] %s에게 %.0f 피해"),
                    *Target->GetName(), SkillDamage);
            }
        }
    }

    // ★ 최대 틱 도달하면 타이머 정리
    if (E_CurrentTick >= E_MaxTicks)
    {
        GetWorldTimerManager().ClearTimer(E_TickTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] DoT 종료"));

        // ★ 추가 — 다음 평타 스턴 플래그 ON
        bNextAttackStun = true;
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] 다음 평타 강화 (스턴)"));

        // 5초간 못 쓰면 만료
        GetWorld()->GetTimerManager().SetTimer(
            E_StunBuffTimerHandle,
            this,
            &AChampion_Alistar::EndStunBuff,
            E_StunBuffDuration,
            false
        );
    }
}

void AChampion_Alistar::EndStunBuff()
{
    if (bNextAttackStun)
    {
        bNextAttackStun = false;
        UE_LOG(LogTemp, Warning, TEXT("[Alistar E] 강화 평타 만료 (사용 안 함)"));
    }
}

void AChampion_Alistar::OnBasicAttackHit(ACharacter* Target)
{
    if (!HasAuthority()) return;
    if (!IsValid(Target) || Target == this) return;

    // 강화 평타 플래그가 켜져 있으면 스턴
    if (bNextAttackStun)
    {
        if (ABaseChampion* Enemy = Cast<ABaseChampion>(Target))
        {
            Enemy->ApplyStun(E_StunDuration);
            UE_LOG(LogTemp, Warning, TEXT("[Alistar E] 강화 평타! %s에게 %.1f초 스턴"),
                *Enemy->GetName(), E_StunDuration);
        }

        // 한 번 쓰고 플래그 끄기
        bNextAttackStun = false;
        GetWorldTimerManager().ClearTimer(E_StunBuffTimerHandle);
    }
}

void AChampion_Alistar::Skill_R()
{
    if (!IsLocallyControlled()) return;

    // 로컬 클라이언트 시점에서 스턴 상태여도 R스킬이면 통과
    if (bIsStunned && !CanCastWhileStunned('r')) return;
    Server_Skill_R();
}

bool AChampion_Alistar::Server_Skill_R_Validate()
{
    return true;
}

void AChampion_Alistar::Server_Skill_R_Implementation()
{
    if (bIsUltActive) return;  // 이미 켜져 있으면 무시

    StartUlt();
    Multicast_PlayRMontage();
}

void AChampion_Alistar::StartUlt()
{
    bIsUltActive = true;

    Multicast_ClearCC();   // ← ClearCCExceptKnockup() 대신 이걸로

    UE_LOG(LogTemp, Warning, TEXT("[Alistar R] 불굴의 의지 발동! %.1f초간 피해 감소"), UltDuration);

    GetWorld()->GetTimerManager().SetTimer(
        UltTimerHandle, this, &AChampion_Alistar::EndUlt, UltDuration, false);
}

void AChampion_Alistar::ClearCCExceptKnockup()
{
    // 스턴 해제
    if (bIsStunned)
    {
        ClearStun();
        UE_LOG(LogTemp, Log, TEXT("[Alistar R] 스턴 해제"));
    }

    // 진행 중인 스턴 타이머도 정리
    if (GetWorldTimerManager().IsTimerActive(StunHandle))
    {
        GetWorldTimerManager().ClearTimer(StunHandle);
    }

    if (bIsKnockedBack)
    {
        EndKnockback();
        if (GetCharacterMovement())
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
}

void AChampion_Alistar::EndUlt()
{
    bIsUltActive = false;
    UE_LOG(LogTemp, Warning, TEXT("[Alistar R] 불굴의 의지 종료"));
}

void AChampion_Alistar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChampion_Alistar, bIsUltActive);
}

void AChampion_Alistar::Multicast_ClearCC_Implementation()
{
    // 스턴 해제
    if (bIsStunned) ClearStun();
    if (GetWorldTimerManager().IsTimerActive(StunHandle))
        GetWorldTimerManager().ClearTimer(StunHandle);

    // 넉백/에어본 해제 (EndKnockback이 플래그+넉백 타이머 정리)
    if (bIsKnockedBack)
    {
        EndKnockback();
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

float AChampion_Alistar::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 궁극기 활성 중 피해 70% 감소 (실제로는 30%만 받음)
    if (bIsUltActive)
    {
        DamageAmount *= (1.f - UltDamageReduction);
    }
    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool AChampion_Alistar::CanCastWhileStunned(uint8 skilltype) const
{
    return skilltype == 'r';
}