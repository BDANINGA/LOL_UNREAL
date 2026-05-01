// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h" 
#include "Champion_Vayne.generated.h"

/**
 * 
 */
UCLASS()
class LOL_UNREAL_API AChampion_Vayne : public ABaseChampion
{
	GENERATED_BODY()

public:
    AChampion_Vayne();

    virtual void Skill_Q() override;
    virtual void Skill_W() override;
    virtual void Skill_E() override;
    virtual void Skill_R() override;

    virtual void Tick(float DeltaTime) override;
protected:
    // --- 베인 Q 관련 변수 ---

    bool bIsDashing = false;

    float DashTime = 0.2f;     // 대시 지속 시간
    float DashElapsed = 0.0f;  // 경과 시간

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q(FVector TargetLocation);
    void Server_Skill_Q_Implementation(FVector TargetLocation);
    bool Server_Skill_Q_Validate(FVector TargetLocation);


    FVector DashStart;

    FVector DashTarget;

    // --- W 스킬 상태 변수 ---
 


    // --- E 스킬 상태 변수 ---

    // 서버에서 물리 처리를 수행할 RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ExecuteCondemn(ACharacter* Target);

    // 에디터에서 조절 가능한 스킬 수치
    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushDistance = 1000.0f; // 밀려날 거리

    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushTime = 0.4f; // 밀려나는 시간 (초)

};
