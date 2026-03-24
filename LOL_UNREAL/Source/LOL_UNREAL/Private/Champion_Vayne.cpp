// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion_Vayne.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"

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
	}
}