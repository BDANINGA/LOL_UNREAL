#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Component/Champion_SkillComponent.h"
#include "Champion_Ezreal.generated.h"

struct FEzrealProjectileDamageData
{
    float Damage = 0.0f;
    TSubclassOf<UDamageType> DamageType = nullptr;
    bool bHitMultiple = false;
    TSet<TWeakObjectPtr<AActor>> DamagedTargets;
};

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

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SpawnEzrealProjectile(
        uint8 ProjectileType,
        FVector StartLocation,
        FVector EndLocation,
        float TravelTime,
        float CollisionRadius,
        float Damage,
        TSubclassOf<UDamageType> DamageType,
        bool bHitMultiple
    );

    UFUNCTION()
    void OnEzrealProjectileOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    void ApplyEzrealLineSkill(
        FVector TargetLocation,
        FSkillData& SkillData,
        int32 SkillLevelIdx,
        float Radius,
        TSubclassOf<UDamageType> DamageType,
        float ADRatio,
        bool bHitMultiple
    );

    class UStaticMesh* GetEzrealProjectileMesh(uint8 ProjectileType);
    void SpawnEzrealProjectileVisual(
        uint8 ProjectileType,
        FVector StartLocation,
        FVector EndLocation,
        float TravelTime,
        float CollisionRadius,
        float Damage,
        TSubclassOf<UDamageType> DamageType,
        bool bHitMultiple
    );

    TMap<AActor*, FEzrealProjectileDamageData> ActiveProjectiles;

    UPROPERTY()
    TObjectPtr<class UStaticMesh> QProjectileMesh;

    UPROPERTY()
    TObjectPtr<class UStaticMesh> WProjectileMesh;

    UPROPERTY()
    TObjectPtr<class UStaticMesh> RProjectileMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float QProjectileSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float WProjectileSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float RProjectileSpeed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float QProjectileVisualRadius = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float WProjectileVisualRadius = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float RProjectileVisualRadius = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    float ProjectileVisualScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FVector QProjectileVisualScale3D = FVector(1.0f, 0.8f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FVector WProjectileVisualScale3D = FVector(0.5f, 0.5f, 0.5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FVector RProjectileVisualScale3D = FVector(1.5f, 2.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FRotator QProjectileRotationOffset = FRotator(0.0f, 90.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FRotator WProjectileRotationOffset = FRotator(-110.0f, 90.0f, 270.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ezreal | Projectile")
    FRotator RProjectileRotationOffset = FRotator(180.0f, 270.0f, 180.0f);
};
