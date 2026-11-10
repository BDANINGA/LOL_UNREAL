#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VayneSilverBoltsVisual.generated.h"
class ACharacter;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
UCLASS()
class LOL_UNREAL_API AVayneSilverBoltsVisual : public AActor
{
 GENERATED_BODY()
public:
 AVayneSilverBoltsVisual();
 void SetStack(ACharacter* InTarget, int32 Count, float Duration);
 virtual void Tick(float DeltaSeconds) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
 UPROPERTY(Replicated) TObjectPtr<ACharacter> Target;
 UPROPERTY(ReplicatedUsing=OnRep_Stack) int32 Stack = 0;
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
 UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Materials;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Dynamic;
 UFUNCTION() void OnRep_Stack();
 float Age = 0.f;
};
