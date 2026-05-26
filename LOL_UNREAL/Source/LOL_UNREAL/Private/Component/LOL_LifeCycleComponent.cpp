// 사망, 리스폰 관련 컴포넌트
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_UIComponent.h"

#include "BaseChampion.h"
#include "LOL_GameModeBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ULOL_LifeCycleComponent::ULOL_LifeCycleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULOL_LifeCycleComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		if (ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>())
		{
			StatComp->OnHpZero.AddUObject(this, &ULOL_LifeCycleComponent::Server_HandleDeath);
		}
	}
	
}
void ULOL_LifeCycleComponent::Server_HandleDeath()
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority()) return;


	// 챔피언인 경우 상태 태그 추가 (※ 향후 미니언에도 범용 태그 컴포넌트를 만들면 이 캐스팅도 제거 가능)
	if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
	{
		if (Champion->HasStatusTag(LOLTags::State_Dead)) return;
		Champion->AddStatusTag(LOLTags::State_Dead);
	}

	Multicast_OnDeath();

	if (bCanRespawn)
	{
		if (ALOL_GameModeBase* GM = Cast<ALOL_GameModeBase>(GetWorld()->GetAuthGameMode()))
		{
			GM->RequestRespawn(Cast<ABaseChampion>(OwnerPawn));
		}
	}
	else
	{
		// 미니언/몬스터는 일정 시간(DespawnDelay) 뒤에 파괴(메모리 해제)
		FTimerHandle DestroyTimer;
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [this]() {
			if (OwnerPawn) OwnerPawn->Destroy();
			}, DespawnDelay, false);
	}
}
void ULOL_LifeCycleComponent::Multicast_OnDeath_Implementation()
{
	if (!OwnerPawn) return;

	if (ULOL_MoveComponent* MoveComp = OwnerPawn->FindComponentByClass<ULOL_MoveComponent>())
	{
		MoveComp->StopMovement();
	}
	if (ULOL_AttackComponent* AttackComp = OwnerPawn->FindComponentByClass<ULOL_AttackComponent>())
	{
		AttackComp->ReceivedCrowdControl();
	}
	if (UPawnMovementComponent* MovementComp = OwnerPawn->GetMovementComponent())
	{
		MovementComp->StopMovementImmediately();
	}


			
	TArray<UPrimitiveComponent*> PrimitiveComps;
	OwnerPawn->GetComponents<UPrimitiveComponent>(PrimitiveComps);
	for (UPrimitiveComponent* Comp : PrimitiveComps)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ULOL_UIComponent* UIComp = OwnerPawn->FindComponentByClass<ULOL_UIComponent>())
	{
		UIComp->GetChampionWidget()->SetVisibility(false);
	}
}
void ULOL_LifeCycleComponent::Respawn()
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority() || !bCanRespawn) return;

	if (ABaseChampion* Champion = Cast<ABaseChampion>(OwnerPawn))
	{
		Champion->RemoveStatusTag(LOLTags::State_Dead);
	}

	if (ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>())
	{
		StatComp->SetHP(StatComp->GetStat().MaxHP);
		StatComp->SetMP(StatComp->GetStat().MaxMP);
	}

	OwnerPawn->SetActorLocation(FVector(-4803.230668, 5708.599208, -1461.45844));
	Multicast_OnRespawn();
}
void ULOL_LifeCycleComponent::Multicast_OnRespawn_Implementation()
{
	if (!OwnerPawn) return;

	TArray<UPrimitiveComponent*> PrimitiveComps;
	OwnerPawn->GetComponents<UPrimitiveComponent>(PrimitiveComps);
	for (UPrimitiveComponent* Comp : PrimitiveComps)
	{
		Comp->SetCollisionProfileName(TEXT("Pawn"));
	}

	if (ULOL_UIComponent* UIComp = OwnerPawn->FindComponentByClass<ULOL_UIComponent>())
	{
		UIComp->GetChampionWidget()->SetVisibility(true);

	}
}