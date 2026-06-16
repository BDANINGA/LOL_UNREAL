#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Component/Champion_SkillComponent.h"
#include "Champion_Ezreal.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Ezreal : public ABaseChampion
{
    GENERATED_BODY()

public:
    AChampion_Ezreal();

    virtual void Skill_Q() override;
    virtual void Skill_W() override;
    virtual void Skill_E() override;
    virtual void Skill_R() override;

protected:
    FTimerHandle UltTimerHandle;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_Q(FVector TargetLocation);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_W(FVector TargetLocation);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_E(FVector TargetLocation);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Skill_R(FVector TargetLocation);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayEzrealSkillMontage(UAnimMontage* Montage, float PlayRate, FRotator NewRotation);

    void ApplyEzrealLineSkill(
        FVector TargetLocation,
        FSkillData& SkillData,
        int32 SkillLevelIdx,
        float Radius,
        TSubclassOf<UDamageType> DamageType,
        float ADRatio,
        bool bHitMultiple
    );
};