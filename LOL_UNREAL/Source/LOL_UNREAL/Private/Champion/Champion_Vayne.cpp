// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion/Champion_Vayne.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"     // 2026 05 01 
#include "GameFramework/DamageType.h"   // 2026 05 01 (UDamageType용)


AChampion_Vayne::AChampion_Vayne()
{
	// 스켈레탈 메쉬 에셋 연결
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> VayneMesh(TEXT("/Game/Level/vain_real/unreal_vain_idle.unreal_vain_idle"));

	if (VayneMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(VayneMesh.Object);

		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

		// 애니메이션 블루프린트 연결
		static ConstructorHelpers::FClassFinder<UAnimInstance> VayneABP(TEXT("/Game/Level/vain_real/abp_unreal_vain.abp_unreal_vain_C"));
		if (VayneABP.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(VayneABP.Class);
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}

		static ConstructorHelpers::FObjectFinder<UAnimMontage> VayneAttackMtg(TEXT("/Game/Level/vain_real/am_attack_1.am_attack_1"));
		if (VayneAttackMtg.Succeeded())
		{
			AttackMontage = VayneAttackMtg.Object;
		}

        // 2026 05 01 (몽타주 생성) 
        static ConstructorHelpers::FObjectFinder<UAnimMontage> VayneQMtg(
            TEXT("/Game/Level/vain_real/AM_Skill_Q.am_Skill_Q"));
        if (VayneQMtg.Succeeded())
        {
            QMontage = VayneQMtg.Object;
        }

        static ConstructorHelpers::FObjectFinder<UAnimMontage> VayneEMtg(
            TEXT("/Game/Level/vain_real/AM_skill_E.am_skill_E"));
        if (VayneEMtg.Succeeded())
        {
            EMontage = VayneEMtg.Object;
        }

        static ConstructorHelpers::FObjectFinder<UAnimMontage> VayneRMtg(
            TEXT("/Game/Level/vain_real/am_vain_ult_idle.am_vain_ult_idle"));
        if (VayneRMtg.Succeeded())
        {
            RMontage = VayneRMtg.Object;
        }
	}
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
    if (QMontage)
    {
        PlayAnimMontage(QMontage, 1.0f);
    }
}

void AChampion_Vayne::Server_Skill_Q_Implementation(FVector QLocation)
{
    if (bIsDashing) return;

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
}

void AChampion_Vayne::Skill_W()
{
    UE_LOG(LogTemp, Log, TEXT("[Vayne W] 은빛 화살은 패시브입니다."));
}

void AChampion_Vayne::OnBasicAttackHit(ACharacter* Target)
{
    // 서버 권한 체크 — 카운트는 서버에서만 굴림
    if (!HasAuthority()) return;
    if (!IsValid(Target) || Target == this) return;

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

    // 1. 추가 피해 적용 (서버에서)
    UGameplayStatics::ApplyDamage(
        Target,
        BoltsBonusDamage,
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

    // 1. 클라이언트(내 화면)에서 타겟팅 수행
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC && PC->IsLocalController())
    {
        FHitResult Hit;
        if (PC->GetHitResultUnderCursor(ECC_Pawn, false, Hit))
        {
            ACharacter* Target = Cast<ACharacter>(Hit.GetActor());

            // 거리 체크 (500)
            if (Target && Target != this && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= 500.0f)
            {
                // ★ 추가 — 본인 화면에서 즉시 회전 (반응성 향상)
                FVector Direction = Target->GetActorLocation() - GetActorLocation();
                Direction.Z = 0.0f;
                if (!Direction.IsNearlyZero())
                {
                    FRotator LookRotation = Direction.Rotation();
                    LookRotation.Pitch = 0.0f;
                    LookRotation.Roll = 0.0f;
                    SetActorRotation(LookRotation);
                }

                // 2. 서버에 실행 요청 (인자로 타겟 전달)
                Server_ExecuteCondemn(Target);
            }
        }
    }
}

void AChampion_Vayne::Multicast_PlayEMontage_Implementation()
{
    if (EMontage)
    {
        PlayAnimMontage(EMontage, 2.0f);
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
    if (!Target || !Target->GetCharacterMovement()) return;

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
    Target->LaunchCharacter(LaunchVelocity, true, true);
}

void AChampion_Vayne::Skill_R()
{
    
}