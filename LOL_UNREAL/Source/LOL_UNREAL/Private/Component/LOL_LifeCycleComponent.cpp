// 사망, 리스폰 관련 컴포넌트
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StateComponent.h"

#include "BaseChampion.h"
#include "LOL_GameModeBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

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
			StatComp->OnHpZero.AddDynamic(this, &ULOL_LifeCycleComponent::Server_HandleDeath);
		}
	}
	
}
void ULOL_LifeCycleComponent::Server_HandleDeath(AController* KillerInstigator, AActor* DamageCauser)
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority()) return;

	if (KillerInstigator)
	{
		APawn* KillerPawn = KillerInstigator->GetPawn();
		if (ABaseChampion* KillerChampion = Cast<ABaseChampion>(KillerPawn))
		{
			ULOL_StatComponent* KillerStatComp = KillerChampion->FindComponentByClass<ULOL_StatComponent>();
			ULOL_StatComponent* MyStatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>();

			if (KillerStatComp && MyStatComp)
			{
				KillerStatComp->AddGold(MyStatComp->GetGiveGold());
				KillerStatComp->AddEXP(MyStatComp->GetGiveEXP());
			}
		}
	}

	if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
	{
		if (!StateComp->HasStatusTag(LOLTags::State_Dead))
		{
			StateComp->AddStatusTag(LOLTags::State_Dead);
		}
	}

	Multicast_OnDeath();

	if (Cast<ABaseChampion>(OwnerPawn))
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
		UIComp->GetActorWidget()->SetVisibility(false);
	}
}
void ULOL_LifeCycleComponent::Respawn()
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority() || !bCanRespawn) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);
	
	AActor* TargetStart = nullptr;

	FName TeamTag = "BlueTeam";
	if (ABaseChampion* OwnerChampion = Cast<ABaseChampion>(OwnerPawn))
	{
		if(OwnerChampion->StateComponent->HasStatusTag(LOLTags::Team_Blue)) TeamTag = "BlueTeam";
		else FName TeamTag = "RedTeam";
	}
	

	for (AActor* Actor : FoundActors)
	{
		if (Actor->ActorHasTag(TeamTag))
		{
			TargetStart = Actor;
			break;
		}
	}

	if (!TargetStart && FoundActors.Num() > 0)
	{
		TargetStart = FoundActors[0];
	}

	if (TargetStart)
	{
		OwnerPawn->TeleportTo(TargetStart->GetActorLocation(), TargetStart->GetActorRotation());
	}

	if (ULOL_StateComponent* StateComp = OwnerPawn->FindComponentByClass<ULOL_StateComponent>())
	{
		StateComp->RemoveStatusTag(LOLTags::State_Dead);
	}

	if (ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>())
	{
		StatComp->SetHP(StatComp->GetStat().MaxHP);
		StatComp->SetMP(StatComp->GetStat().MaxMP);
	}

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
		UIComp->GetActorWidget()->SetVisibility(true);

	}
}