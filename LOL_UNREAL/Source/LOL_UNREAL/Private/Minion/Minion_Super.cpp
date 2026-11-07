// 슈퍼 미니언
#include "Minion/Minion_Super.h"
#include "Components/SkeletalMeshComponent.h"

AMinion_Super::AMinion_Super()
{
	MinionName = TEXT("Minion_Super");
	SetMinionData(MinionName);
	GetMesh()->SetRelativeScale3D(FVector::OneVector);
}
