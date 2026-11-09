#include "Champion/VayneSilverBoltsVisual.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
AVayneSilverBoltsVisual::AVayneSilverBoltsVisual()
{
 bReplicates=true;
 bAlwaysRelevant=true;
 PrimaryActorTick.bCanEverTick=true;
 Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SilverBoltsRing"));
 SetRootComponent(Ring);
 Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Ring->SetGenerateOverlapEvents(false);
 Ring->SetCanEverAffectNavigation(false);
 Ring->SetCastShadow(false);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
 Ring->SetStaticMesh(Plane.Object);
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> M1(TEXT("/Game/VFX/Vayne/M_VayneSilverBolts1.M_VayneSilverBolts1"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> M2(TEXT("/Game/VFX/Vayne/M_VayneSilverBolts2.M_VayneSilverBolts2"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> M3(TEXT("/Game/VFX/Vayne/M_VayneSilverBolts3.M_VayneSilverBolts3"));
 Materials={M1.Object,M2.Object,M3.Object};
 Ring->SetVisibility(false);
}
void AVayneSilverBoltsVisual::SetStack(ACharacter* InTarget,int32 Count,float Duration)
{
 if(!HasAuthority()) return;
 Target=InTarget;
 Stack=FMath::Clamp(Count,1,3);
 OnRep_Stack();
 SetLifeSpan(Stack==3 ? .55f : FMath::Max(Duration,.1f));
 ForceNetUpdate();
}
void AVayneSilverBoltsVisual::OnRep_Stack()
{
 Age=0.f;
 if(GetNetMode()==NM_DedicatedServer || !Materials.IsValidIndex(Stack-1)) return;
 Dynamic=Ring->CreateDynamicMaterialInstance(0,Materials[Stack-1]);
 Ring->SetVisibility(true);
}
void AVayneSilverBoltsVisual::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 Age+=DeltaSeconds;
 if(!IsValid(Target))
 {
  Ring->SetVisibility(false);
  if(HasAuthority()) Destroy();
  return;
 }
 FVector Foot=Target->GetActorLocation();
 Foot.Z-=Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 Foot.Z+=5.f;
 SetActorLocation(Foot);
 const float Radius=FMath::Max(65.f,Target->GetCapsuleComponent()->GetScaledCapsuleRadius()*1.65f);
 const float Burst=Stack==3 ? FMath::Clamp(Age/.55f,0.f,1.f) : 0.f;
 Ring->SetWorldScale3D(FVector(Radius/50.f*(1.f+Burst*.8f)));
 if(Dynamic) Dynamic->SetScalarParameterValue(TEXT("Brightness"),Stack==3 ? 8.f*(1.f-Burst) : 2.8f+.3f*FMath::Sin(Age*4.f));
}
void AVayneSilverBoltsVisual::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(AVayneSilverBoltsVisual,Target);
 DOREPLIFETIME(AVayneSilverBoltsVisual,Stack);
}
