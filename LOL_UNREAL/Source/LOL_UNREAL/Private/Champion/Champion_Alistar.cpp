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

void AChampion_Alistar::SetChampionData(FName RowName)
{
    static ConstructorHelpers::FObjectFinder<UDataTable> ChampionResource(TEXT("/Game/LOL_Data/Data_Champions/Data_ChampionResource.Data_ChampionResource"));
    if (ChampionResource.Succeeded())
    {
        UDataTable* DataTable = ChampionResource.Object;

        FChampionData* Data = DataTable->FindRow<FChampionData>(RowName, TEXT(""));

        if (Data)
        {
            if (Data->Mesh)
            {
                GetMesh()->SetSkeletalMesh(Data->Mesh);
                GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
                GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
            }
            if (Data->AnimBlueprint)
            {
                GetMesh()->SetAnimInstanceClass(Data->AnimBlueprint);
                GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            }
            Portrait = Data->Portrait;
            Portrait_Circle = Data->Portrait_Circle;
            Portrait_Loading = Data->Portrait_Loading;

            SkillQ_Image = Data->SkillQ_Image;
            SkillW_Image = Data->SkillW_Image;
            SkillE_Image = Data->SkillE_Image;
            SkillR_Image = Data->SkillR_Image;
            SkillP_Image = Data->SkillP_Image;

            AttackMontage = Data->AttackMontage;
            QMontage = Data->QMontage;
            WMontage = Data->WMontage;
            EMontage = Data->EMontage;
            RMontage = Data->RMontage;
            PMontage = Data->PMontage;
        }
    }
}

void AChampion_Alistar::Multicast_PlayQMontage_Implementation()
{
    if (QMontage)
    {
        PlayAnimMontage(QMontage, 2.0f);
    }
}

void AChampion_Alistar::Multicast_PlayWMontage_Implementation()
{
    if (WMontage)
    {
        PlayAnimMontage(WMontage, 2.0f);
    }
}

void AChampion_Alistar::Multicast_PlayRMontage_Implementation()
{
    if (RMontage)
    {
        PlayAnimMontage(RMontage, 1.0f);
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
    // ★ 본인 시전 잠금
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();


    Multicast_PlayQMontage();
    // 2. 범위 설정
    FVector Center = GetActorLocation();

    // 3. 충돌 결과 저장
    TArray<FHitResult> Hits;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(300.0f);

    bool bHit = GetWorld()->SweepMultiByChannel(
        Hits,
        GetActorLocation(),
        GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        Sphere
    );

    if (bHit)
    {
        for (auto& Hit : Hits)
        {
            ACharacter* Target = Cast<ACharacter>(Hit.GetActor());

            if (Target && Target != this)
            {
                // 1. 공격 애니메이션 즉시 중단 (매우 중요)
                Target->StopAnimMontage();

                UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement();
                if (MoveComp)
                {
                    // 2. 기존 운동 에너지 및 루트 모션 데이터 제거
                    MoveComp->StopMovementImmediately();
                    MoveComp->CurrentRootMotion.Clear();

                    // 3. 공중 상태로 강제 전환 (에어본이 가능하도록)
                    MoveComp->SetMovementMode(MOVE_Falling);
                }

                // ★ 추가 — 적이 알리스타 캡슐을 무시하도록 (옆으로 안 밀리게)
                Target->MoveIgnoreActorAdd(this);

                // 4. 에어본 발사
                Target->LaunchCharacter(FVector(0, 0, 600.0f), true, true);

                if (ABaseChampion* Champ = Cast<ABaseChampion>(Target))
                {
                    // W에서 썼던 것과 동일하게 IsKnockedBack(또는 IsAirborne) 상태를 켜주는 것이 좋습니다.
                    // 그래야 상대방 Tick 로직에서 다시 이동을 시도하지 못합니다.
                    Champ->SetIsKnockedBack(true);
                    Champ->ApplyStun(2.0f);
           
                    // 2초 뒤(스턴 종료 시) 상태 복구 타이머 추가 필요
                    FTimerHandle AirTimer;
                    GetWorld()->GetTimerManager().SetTimer(AirTimer, [Champ, this]() {
                        if (IsValid(Champ))
                        {
                            Champ->SetIsKnockedBack(false); 
                            
                            // ★ 추가 — 충돌 무시 해제
                            if (IsValid(this))
                            {
                                Champ->MoveIgnoreActorRemove(this);
                            }
                        }
                        }, 1.0f, false); // 에어본 체공 시간만큼 설정
                }

                float SkillDamage = SkillComponent->GetQ_Data().BaseDamage[0] + StatComponent->GetStat().AttackDamage * 0.8f;

                UGameplayStatics::ApplyDamage(
                    Target,
                    SkillDamage,
                    this->GetController(),
                    this,
                    ULOL_DamageMagic::StaticClass()
                );
            }
        }
    }

    // ★ Q_CastTime(0.5초) 후 이동 복구
    GetWorld()->GetTimerManager().SetTimer(
        Q_CastTimerHandle,
        this,
        &AChampion_Alistar::EndQCast,
        Q_CastTime,
        false
    );
}

bool AChampion_Alistar::Server_Skill_W(AActor* Target)
{
    if (!IsValid(Target)) return false;
    return true;
}

void AChampion_Alistar::Skill_W()
{
    // 1. 마우스 아래의 캐릭터 타겟팅
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    FHitResult HitResult;
    // 마우스 커서 아래의 'Pawn' 또는 'Visibility' 채널 체크
    if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
    {
        ACharacter* Target = Cast<ACharacter>(HitResult.GetActor());
        // 2. 유효성 검사 (타겟 존재 여부, 자기 자신 제외)
        if (!Target || Target == this) return;
        // 3. 사거리 체크
        float SkillRange = 550.0f; // 알리스타 W 평균 사거리
        float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
        if (Distance <= SkillRange)
        {
            // 타겟 저장 (추적용)
            CurrentWTarget = Target;
            // 돌진 상태 시작
            bIsW_Dashing = true;
            // 4. 돌진 방향 및 속도 계산
            FVector DashDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            float DashSpeed = 1200.0f;

            FRotator LookRotation = DashDirection.Rotation();
            LookRotation.Pitch = 0.0f;
            LookRotation.Roll = 0.0f;
            SetActorRotation(LookRotation);

            // 돌진 시작
            LaunchCharacter(DashDirection * DashSpeed, true, true);

            Multicast_PlayWMontage();
        }

        float SkillDamage = SkillComponent->GetW_Data().BaseDamage[0] + StatComponent->GetStat().AbilityPower * 1.0f;

        UGameplayStatics::ApplyDamage(
            Target,
            SkillDamage,
            this->GetController(),
            this,
            ULOL_DamageMagic::StaticClass()
        );
    }
}

void AChampion_Alistar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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

    // ★ CC 해제 — 에어본 빼고
    ClearCCExceptKnockup();

    UE_LOG(LogTemp, Warning, TEXT("[Alistar R] 불굴의 의지 발동! %.1f초간 무적"), UltDuration);

    // 7초 후 자동 종료
    GetWorld()->GetTimerManager().SetTimer(
        UltTimerHandle,
        this,
        &AChampion_Alistar::EndUlt,
        UltDuration,
        false
    );
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

    // ❗ bIsKnockedBack은 건드리지 않음 — 에어본은 유지
    // 만약 베인 E 같은 다른 CC가 추가되면 여기서 같이 해제
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