#pragma once

#include "CoreMinimal.h"
#include "BaseChampion.h"
#include "Champion_Alistar.generated.h"

UCLASS()
class LOL_UNREAL_API AChampion_Alistar : public ABaseChampion
{
	GENERATED_BODY()

public:
	AChampion_Alistar();

	virtual void SetChampionData(FName RowName) override;

	virtual void Skill_Q() override;
	virtual void Skill_W() override;
	virtual void Skill_R() override;

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")

	TObjectPtr<class ACharacter> CurrentWTarget;
	bool bIsW_Dashing = false;

	void ApplyWKnockback(ACharacter* Target);

	bool Server_Skill_W(AActor* Target);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayQMontage();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayWMontage();
	
	// --- R 스킬: 불굴의 의지 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDuration = 7.0f;  // 지속시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|R")
	float UltDamageReduction = 0.5f;  // 받는 피해 50% 감소

	UPROPERTY(Replicated)
	bool bIsUltActive = false;

	FTimerHandle UltTimerHandle;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Skill_R();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayRMontage();

	void StartUlt();
	void EndUlt();

	void ClearCCExceptKnockup();
};
