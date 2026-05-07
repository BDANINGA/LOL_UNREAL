// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion/Champion_Blitz.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"


AChampion_Blitz::AChampion_Blitz()
{
    // 1. 스켈레탈 메쉬 로드
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> BlitzMesh(
        TEXT("/Game/Level/blitzcrank/blitzcrank_mesh.blitzcrank_mesh"));

    if (BlitzMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(BlitzMesh.Object);

        // 캡슐과 메쉬 위치 맞추기 (베인/알리스타와 동일 패턴)
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

        // 2. 애니메이션 블루프린트 연결
        static ConstructorHelpers::FClassFinder<UAnimInstance> BlitzABP(
            TEXT("/Game/Level/blitzcrank/abp_blitzcrank.abp_blitzcrank_C"));
        if (BlitzABP.Succeeded())
        {
            GetMesh()->SetAnimInstanceClass(BlitzABP.Class);
            GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        }

        // 3. 평타 몽타주
        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzAttackMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_attack.am_blitz_attack"));
        if (BlitzAttackMtg.Succeeded())
        {
            AttackMontage = BlitzAttackMtg.Object;
        }

        // 4. 죽음 몽타주
        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzDeathMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_death.am_blitz_death"));
        if (BlitzDeathMtg.Succeeded())
        {
            DeathMontage = BlitzDeathMtg.Object;
        }

        // 5. 스킬 몽타주들 (Q, W, E, R)
        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzQMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_q.am_blitz_q"));
        if (BlitzQMtg.Succeeded())
        {
            QMontage = BlitzQMtg.Object;
        }

        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzWMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_w.am_blitz_w"));
        if (BlitzWMtg.Succeeded())
        {
            WMontage = BlitzWMtg.Object;
        }

        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzEMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_e.am_blitz_e"));
        if (BlitzEMtg.Succeeded())
        {
            EMontage = BlitzEMtg.Object;
        }

        static ConstructorHelpers::FObjectFinder<UAnimMontage> BlitzRMtg(
            TEXT("/Game/Level/blitzcrank/am_blitz_r.am_blitz_r"));
        if (BlitzRMtg.Succeeded())
        {
            RMontage = BlitzRMtg.Object;
        }
    }
}

// 빈 함수들 (스킬 로직은 나중에)
void AChampion_Blitz::Skill_Q() {}
void AChampion_Blitz::Skill_W() {}
void AChampion_Blitz::Skill_E() {}
void AChampion_Blitz::Skill_R() {}

void AChampion_Blitz::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}