// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion_Alistar.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"

AChampion_Alistar::AChampion_Alistar()
{
	// 스켈레탈 메쉬 에셋 연결
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> AlistarMesh(TEXT("/Game/Level/alistar/unrea_alistar_real_1.unrea_alistar_real_1"));

	if (AlistarMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(AlistarMesh.Object);

		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

		// 애니메이션 블루프린트 연결
		static ConstructorHelpers::FClassFinder<UAnimInstance> AlistarABP(TEXT("/Game/Level/alistar/abp_alistar.abp_alistar_C"));
		if (AlistarABP.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(AlistarABP.Class);
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		}

		static ConstructorHelpers::FObjectFinder<UAnimMontage> AlistarAttackMtg(TEXT("/Game/Level/alistar/am_alistar_attack1.am_alistar_attack1"));
		if (AlistarAttackMtg.Succeeded())
		{
			AttackMontage = AlistarAttackMtg.Object;
		}

		static ConstructorHelpers::FObjectFinder<UAnimMontage> AlistarDeathMtg(TEXT("/Game/Level/alistar/am_alistar_death.am_alistar_death"));
		if (AlistarDeathMtg.Succeeded())
		{
			DeathMontage = AlistarDeathMtg.Object;
		}
	}
}