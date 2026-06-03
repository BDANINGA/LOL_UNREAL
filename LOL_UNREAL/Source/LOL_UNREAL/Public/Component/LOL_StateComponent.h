// 상태 컴포넌트 (게임플레이 태그)
#pragma once

#include "GamePlayTag/LOL_GamePlayTags.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOL_StateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LOL_UNREAL_API ULOL_StateComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:	
	ULOL_StateComponent();

	UFUNCTION(BlueprintCallable, Category = "State")
	void AddStatusTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "State")
	void RemoveStatusTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "State")
	bool HasStatusTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "State")
	bool IsEnemy(ULOL_StateComponent* OtherState) const;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:	
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_StatusTags)
	FGameplayTagContainer StatusTags;

	UFUNCTION()
	void OnRep_StatusTags();
};
