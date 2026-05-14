// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VayneArrow.generated.h"

UCLASS()
class LOL_UNREAL_API AVayneArrow : public AActor
{
    GENERATED_BODY()

public:
    AVayneArrow();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    /** 발사 시 시작/도착 위치 설정 */
    void InitArrow(FVector InStart, FVector InTarget);

protected:
    /** 화살 메쉬 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UStaticMeshComponent* ArrowMesh;

    /** 비행 시간 (시작에서 목적지까지 걸리는 초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
    float FlyTime = 1.0f;

    /** 시작 위치 */
    FVector StartLocation;

    /** 목적지 위치 */
    FVector TargetLocation;

    /** 경과 시간 */
    float Elapsed = 0.0f;
};