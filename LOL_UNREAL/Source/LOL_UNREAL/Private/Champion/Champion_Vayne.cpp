// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion/Champion_Vayne.h"
#include "Components/CapsuleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


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

		/*static ConstructorHelpers::FObjectFinder<UAnimMontage> VayneDeathMtg(TEXT("/Game/Level/vain_real/am_attack_1.am_attack_1"));
		if (VayneDeathMtg.Succeeded())
		{
			DeathMontage = VayneDeathMtg.Object;
		}*/
	}
}

void AChampion_Vayne::Skill_Q()
{
    if (!IsLocallyControlled()) return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    // 서버에 대시 요청
    Server_Skill_Q(Hit.Location);
}

bool AChampion_Vayne::Server_Skill_Q_Validate(FVector TargetLocation) { return true; }

void AChampion_Vayne::Server_Skill_Q_Implementation(FVector TargetLocation)
{
    FVector Start = GetActorLocation();

    // 1. 수평 방향 계산 (Z값 무시)
    FVector Direction = TargetLocation - Start;
    Direction.Z = 0.0f;
    Direction.Normalize();

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

}

void AChampion_Vayne::Skill_E()
{
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
                // 2. 서버에 실행 요청 (인자로 타겟 전달)
                Server_ExecuteCondemn(Target);
            }
        }
    }
}

// 서버 RPC 검증
bool AChampion_Vayne::Server_ExecuteCondemn_Validate(ACharacter* Target)
{
    return true;
}

// 서버 RPC 실제 실행 (AChampion_Vayne:: 를 반드시 붙여야 함)
void AChampion_Vayne::Server_ExecuteCondemn_Implementation(ACharacter* Target)
{
    if (!Target || !Target->GetCharacterMovement()) return;

    // 3. 물리 계산 (서버에서 수행)
    FVector MyLoc = GetActorLocation();
    FVector TargetLoc = Target->GetActorLocation();

    // 밀어낼 방향 (나 -> 상대)
    FVector PushDir = (TargetLoc - MyLoc).GetSafeNormal2D();

    // 4. 발사 속도 계산 (속도 = 거리 / 시간)
    // 0.4초 동안 600 유닛을 이동시키기 위한 초기 속도 설정
    FVector LaunchVelocity = (PushDir * PushDistance) / PushTime;

    // 공중에 살짝 뜨는 느낌을 주고 싶다면 Z축 값 추가 (선택)
    // LaunchVelocity.Z = 300.0f;

    // 5. 핵심 함수 실행
    // XYOverride, ZOverride를 true로 설정하여 기존 이동 속도를 무시하고 즉시 발사
    Target->LaunchCharacter(LaunchVelocity, true, true);

    // (참고) LaunchCharacter는 서버에서 호출 시 CharacterMovementComponent를 통해 
    // 모든 클라이언트에 위치와 가속도가 자동으로 동기화됩니다.
}

void AChampion_Vayne::Skill_R()
{

}