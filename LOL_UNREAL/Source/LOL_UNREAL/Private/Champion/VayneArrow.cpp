// Fill out your copyright notice in the Description page of Project Settings.
#include "Champion/VayneArrow.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AVayneArrow::AVayneArrow()
{
    PrimaryActorTick.bCanEverTick = true;

    // 메쉬 컴포넌트 생성
    ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("/Game/Level/vain_real/vain_attack_common_arrow.vain_attack_common_arrow"));
    RootComponent = ArrowMesh;

    // 충돌 끄기 (시각 효과 전용이라 충돌 불필요)
    ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 멀티플레이 동기화
    bReplicates = true;
}

void AVayneArrow::BeginPlay()
{
    Super::BeginPlay();
}

void AVayneArrow::InitArrow(FVector InStart, FVector InTarget)
{
    StartLocation = InStart;
    TargetLocation = InTarget;

    SetActorLocation(StartLocation);

    // 진행 방향으로 회전 (화살이 비행 방향을 향하도록)
    FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        SetActorRotation(Direction.Rotation());
    }
}

void AVayneArrow::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    Elapsed += DeltaTime;
    float Alpha = FMath::Clamp(Elapsed / FlyTime, 0.0f, 1.0f);

    // 시작점에서 목적지까지 보간 이동
    FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);
    SetActorLocation(NewLocation);

    // 도착하면 자동 소멸
    if (Alpha >= 1.0f)
    {
        Destroy();
    }
}