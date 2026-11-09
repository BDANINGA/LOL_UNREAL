#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VayneTumbleVisualComponent.generated.h"
UCLASS()
class LOL_UNREAL_API UVayneTumbleVisualComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UVayneTumbleVisualComponent();
 UFUNCTION(BlueprintCallable) void PlayVisual();
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
protected:
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
 void ClearVisual();
 UPROPERTY() TObjectPtr<class UStaticMesh> Plane;
 UPROPERTY() TObjectPtr<class UMaterialInterface> WingMaterial;
 UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Wings;
 UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> Materials;
 FVector Origin=FVector::ZeroVector;
 float Age=0.f;
};
