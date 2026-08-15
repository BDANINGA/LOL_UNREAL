// 챔피언 스킬 관련 컴포넌트
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Champion_SkillComponent.generated.h"

USTRUCT(BlueprintType)
struct FSkillData : public FTableRowBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    FString SkillName;

    UPROPERTY(EditAnywhere)
    TArray<float> BaseDamage;

    UPROPERTY(EditAnywhere)
    TArray<float> ManaCost;

    UPROPERTY(EditAnywhere)
    TArray<float> Cooldown;

    UPROPERTY(EditAnywhere)
    TArray<float> Range;

    UPROPERTY(EditAnywhere)
    TArray<float> Duration; // CC기 또는 효과 지속시간

    UPROPERTY(EditAnywhere)
    float CastTime; // 선딜레이 (단일값)

    UPROPERTY(EditAnywhere)
    TArray<float> SecondaryValue; // 넉백 거리, 피해 감소율 등 특수 수치

    UPROPERTY(EditAnywhere)
    float CooldownEndTime{};    // 쿨타임 종료 시간
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API UChampion_SkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UChampion_SkillComponent();

	void InitializeSkills();

    FSkillData& GetQ_Data() { return Q_Data; }
    FSkillData& GetW_Data() { return W_Data; }
    FSkillData& GetE_Data() { return E_Data; }
    FSkillData& GetR_Data() { return R_Data; }

    bool TryCastSkill(FName SkillName, int32 SkillLevel = -1);
    int32 GetSkillLevel(FName SkillName) const;
    int32 GetSkillLevelIndex(FName SkillName) const;
    int32 GetAvailableSkillPoints() const { return AvailableSkillPoints; }
    bool CanLevelUpSkill(FName SkillName) const;
    void AddSkillPointForChampionLevel(int32 ChampionLevel);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_LevelUpSkill(FName SkillName);

    class ABaseChampion* Owner;
    
protected:
	virtual void BeginPlay() override;

    UFUNCTION(Client, Reliable)
    void Client_UpdateHUDCooldown(FName SkillName, float CoolLocalEndTime, float CoolEndTime);

private:
    UPROPERTY(EditAnywhere, Category = "Data")
    class UDataTable* SkillDataTable;

    FSkillData Q_Data;
    FSkillData W_Data;
    FSkillData E_Data;
    FSkillData R_Data;

    UPROPERTY(Replicated)
    int32 QSkillLevel = 0;

    UPROPERTY(Replicated)
    int32 WSkillLevel = 0;

    UPROPERTY(Replicated)
    int32 ESkillLevel = 0;

    UPROPERTY(Replicated)
    int32 RSkillLevel = 0;

    UPROPERTY(Replicated)
    int32 AvailableSkillPoints = 0;

    int32 GetMaxAllowedSkillLevel(FName SkillName, int32 ChampionLevel) const;
    int32& GetMutableSkillLevel(FName SkillName);
    void ApplyCurrentSkillRankData(FName SkillName);
    void CopyCurrentRankToFirstIndex(FSkillData& SkillData, int32 SkillLevel);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
