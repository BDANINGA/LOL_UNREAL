#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BaseBuilding.generated.h"

UCLASS()
class LOL_UNREAL_API ABaseBuilding : public APawn
{
	GENERATED_BODY()

public:
	ABaseBuilding();

	virtual FName GetBuildingName() const { return BuildingName; };

	UFUNCTION()
	void UpdateTeamVisual();

	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULOL_StatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULOL_StateComponent> StateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULOL_AttackComponent> AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULOL_LifeCycleComponent> LifeCycleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULOL_UIComponent> UIComponent;

	UPROPERTY(EditAnywhere, Category = "Material")
	UTexture2D* AllyTexture;

	UPROPERTY(EditAnywhere, Category = "Material")
	UTexture2D* EnemyTexture;

	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* AllyHPBarImage;

	UPROPERTY(EditAnywhere, Category = "UI")
	UTexture2D* EnemyHPBarImage;

	UPROPERTY(EditDefaultsOnly, Category = "Building|Data")
	FName BuildingName;
};