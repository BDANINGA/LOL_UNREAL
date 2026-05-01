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
    // --- ���� Q ���� ���� ---

    bool bIsDashing = false;

    float DashTime = 0.2f;     // ��� ���� �ð�
    float DashElapsed = 0.0f;  // ��� �ð�

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q(FVector QLocation);
    void Server_Skill_Q_Implementation(FVector QLocation);
    bool Server_Skill_Q_Validate(FVector QLocation);


    FVector DashStart;

    FVector DashTarget;

    // --- W ��ų ���� ���� ---
 


    // --- E ��ų ���� ���� ---

    // �������� ���� ó���� ������ RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ExecuteCondemn(ACharacter* Target);

    // �����Ϳ��� ���� ������ ��ų ��ġ
    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushDistance = 1000.0f; // �з��� �Ÿ�

    UPROPERTY(EditAnywhere, Category = "Skill|E")
    float PushTime = 0.4f; // �з����� �ð� (��)

};
