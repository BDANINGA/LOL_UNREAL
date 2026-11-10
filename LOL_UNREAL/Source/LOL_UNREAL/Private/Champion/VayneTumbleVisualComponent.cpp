#include "Champion/VayneTumbleVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
UVayneTumbleVisualComponent::UVayneTumbleVisualComponent()
{
 PrimaryComponentTick.bCanEverTick=true;
 PrimaryComponentTick.bStartWithTickEnabled=false;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> P(TEXT("/Engine/BasicShapes/Plane.Plane"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> M(TEXT("/Game/VFX/Vayne/M_VayneTumbleWing.M_VayneTumbleWing"));
 Plane=P.Object;WingMaterial=M.Object;
}
void UVayneTumbleVisualComponent::PlayVisual()
{
 ClearVisual();
 if(GetNetMode()==NM_DedicatedServer || !Plane || !WingMaterial) return;
 Origin=GetOwner()->GetActorLocation();Age=0.f;
 for(int32 i=0;i<12;++i)
 {
  auto* Wing=NewObject<UStaticMeshComponent>(GetOwner());
  Wing->SetStaticMesh(Plane);
  Wing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Wing->SetGenerateOverlapEvents(false);
  Wing->SetCanEverAffectNavigation(false);
  Wing->SetCastShadow(false);
  Wing->SetWorldLocation(Origin);
  Wing->RegisterComponent();
  Wings.Add(Wing);
  auto* Material=Wing->CreateDynamicMaterialInstance(0,WingMaterial);
  Materials.Add(Material);
  if(Material) Material->SetVectorParameterValue(TEXT("Tint"), i%3==0 ? FLinearColor(.035f,.008f,.065f) : FLinearColor(.34f,.035f,.65f));
 }
 SetComponentTickEnabled(true);
 TickComponent(0.f,LEVELTICK_All,nullptr);
}
void UVayneTumbleVisualComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
 Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
 Age+=DeltaTime;
 if(Age>=.7f){ClearVisual();return;}
 const FVector Current=GetOwner()->GetActorLocation();
 for(int32 i=0;i<Wings.Num();++i)
 {
  const float Phase=i*2.39996f;
  const float A=Phase+Age*(i%2==0 ? 5.f : -4.f);
  const float Spread=32.f+Age*(75.f+(i%3)*22.f);
  const FVector Center=FMath::Lerp(Origin,Current,.45f+.045f*i);
  Wings[i]->SetWorldLocation(Center+FVector(FMath::Cos(A)*Spread,FMath::Sin(A)*Spread,-35.f+(i%4)*18.f+Age*48.f));
  Wings[i]->SetWorldRotation(FRotator(15.f*FMath::Sin(A),FMath::RadiansToDegrees(A),20.f*FMath::Sin(Age*35.f+Phase)));
  const float Flap=.28f+.72f*FMath::Abs(FMath::Sin(Age*30.f+Phase));
  const float Size=.32f+.055f*(i%4);
  Wings[i]->SetWorldScale3D(FVector(Size*Flap,Size,1.f));
  if(Materials[i]) Materials[i]->SetScalarParameterValue(TEXT("Fade"),FMath::Clamp(Age/.035f,0.f,1.f)*(1.f-FMath::SmoothStep(.3f,.7f,Age)));
 }
}
void UVayneTumbleVisualComponent::ClearVisual()
{
 for(auto Wing:Wings) if(IsValid(Wing)) Wing->DestroyComponent();
 Wings.Empty();Materials.Empty();SetComponentTickEnabled(false);
}
void UVayneTumbleVisualComponent::EndPlay(const EEndPlayReason::Type Reason)
{
 ClearVisual();Super::EndPlay(Reason);
}

