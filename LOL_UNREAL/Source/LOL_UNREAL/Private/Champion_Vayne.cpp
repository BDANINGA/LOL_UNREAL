// Fill out your copyright notice in the Description page of Project Settings.


#include "Champion_Vayne.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"

AChampion_Vayne::AChampion_Vayne()
{
	// 스켈레탈 메쉬 에셋 연결
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> VayneMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Level/vain_real/unreal_vain_idle.unreal_vain_idle'"));

	if (VayneMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(VayneMesh.Object);

		// 2. 캐릭터 캡슐 안에 메쉬 위치 및 회전 정렬
		// 보통 아래로 90유닛, 앞(X축)을 바라보게 -90도 회전 시킵니다.
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}
}