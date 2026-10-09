#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_SummonerSpellComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOL_UNREAL_API ULOL_SummonerSpellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULOL_SummonerSpellComponent();

	void CastExhaust(AActor* Target);
	void CastFlash(const FVector& TargetLocation);
	float GetOutgoingDamageMultiplier() const { return OutgoingDamageMultiplier; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Exhaust")
	float ExhaustRange = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Exhaust")
	float ExhaustDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Exhaust", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExhaustMoveSpeedMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Exhaust", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExhaustDamageMultiplier = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Exhaust", meta = (ClampMin = "0.0"))
	float ExhaustCooldown = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Flash")
	float FlashRange = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoner Spell|Flash", meta = (ClampMin = "0.0"))
	float FlashCooldown = 300.0f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void Server_CastExhaust(AActor* Target);

	UFUNCTION(Server, Reliable)
	void Server_CastFlash(FVector_NetQuantize TargetLocation);

	UFUNCTION(Client, Reliable)
	void Client_StartSummonerSpellCooldown(FName SpellName, float CooldownDuration);

	UFUNCTION(Client, Reliable)
	void Client_SetExhaustMoveSpeed(float MoveSpeed);

private:
	bool CanCastSummonerSpell() const;
	void ApplyExhaustEffect(float Duration, float MoveSpeedMultiplier, float DamageMultiplier);
	void ClearExhaustEffect();

	UPROPERTY()
	class ABaseChampion* OwnerChampion = nullptr;

	float ExhaustCooldownEndTime = 0.0f;
	float FlashCooldownEndTime = 0.0f;
	float OutgoingDamageMultiplier = 1.0f;
	float PreExhaustMoveSpeed = 0.0f;
	float ExhaustAppliedMoveSpeed = 0.0f;
	bool bExhaustActive = false;

	FTimerHandle ExhaustEffectTimerHandle;
};
