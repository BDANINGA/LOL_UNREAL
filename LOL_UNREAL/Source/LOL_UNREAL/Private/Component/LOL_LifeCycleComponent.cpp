// 사망, 리스폰 관련 컴포넌트
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StatComponent.h"
#include "Component/LOL_MoveComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_UIComponent.h"
#include "Component/LOL_StateComponent.h"

#include "BaseChampion.h"
#include "LOL_GameModeBase.h"
#include "LOL_GameState.h"
#include "LOL_PlayerState.h"
#include "JungleMonster/BaseJungleMonster.h"
#include "Minion/BaseMinion.h"

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
		InitialSpawnLocation = OwnerPawn->GetActorLocation();
		InitialSpawnRotation = OwnerPawn->GetActorRotation();

		if (ULOL_StatComponent* StatComp = OwnerPawn->FindComponentByClass<ULOL_StatComponent>())
		{
			StatComp->OnHpZero.AddDynamic(this, &ULOL_LifeCycleComponent::Server_HandleDeath);
		}

		TArray<UPrimitiveComponent*> PrimitiveComps;
		OwnerPawn->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		for (UPrimitiveComponent* Comp : PrimitiveComps)
		{
			if (Comp)
			{
				InitialCollisionStates.Add(Comp, Comp->GetCollisionEnabled());
			}
		}
	}
	
}

void ULOL_LifeCycleComponent::RecordDamageFrom(AController* DamageInstigator)
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority() || !DamageInstigator)
	{
		return;
	}

	ABaseChampion* DamagedChampion = Cast<ABaseChampion>(OwnerPawn);
	ABaseChampion* DamagingChampion = Cast<ABaseChampion>(DamageInstigator->GetPawn());
	if (!DamagedChampion || !DamagingChampion || DamagedChampion == DamagingChampion)
	{
		return;
	}

	if (!DamagedChampion->StateComponent || !DamagingChampion->StateComponent ||
		!DamagedChampion->StateComponent->IsEnemy(DamagingChampion->StateComponent))
	{
		return;
	}

	RecentDamageContributors.FindOrAdd(DamageInstigator) = GetWorld()->GetTimeSeconds();
}

void ULOL_LifeCycleComponent::Server_HandleDeath(AController* KillerInstigator, AActor* DamageCauser)
{
	if (!OwnerPawn || !OwnerPawn->HasAuthority()) return;

	ABaseChampion* KillerChampion = KillerInstigator
		? Cast<ABaseChampion>(KillerInstigator->GetPawn())
		: nullptr;
	ALOL_PlayerState* KillerPlayerState = KillerInstigator
		? KillerInstigator->GetPlayerState<ALOL_PlayerState>()
		: nullptr;

	if (KillerInstigator)
	{
		APawn* KillerPawn = KillerInstigator->GetPawn();
		if (KillerChampion)
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

	if (ABaseChampion* DeadChampion = Cast<ABaseChampion>(OwnerPawn))
	{
		AController* DeadController = DeadChampion->GetController();
		ALOL_PlayerState* DeadPlayerState = DeadController
			? DeadController->GetPlayerState<ALOL_PlayerState>()
			: nullptr;

		if (DeadPlayerState)
		{
			DeadPlayerState->AddDeath();
		}
		DeadChampion->AddDeathCount();

		const bool bValidEnemyKill =
			KillerChampion &&
			KillerChampion != DeadChampion &&
			KillerChampion->StateComponent &&
			DeadChampion->StateComponent &&
			KillerChampion->StateComponent->IsEnemy(DeadChampion->StateComponent);

		if (bValidEnemyKill)
		{
			if (KillerPlayerState)
			{
				KillerPlayerState->AddKill();
			}
			KillerChampion->AddKillCount();

			if (ALOL_GameState* GameState = GetWorld()->GetGameState<ALOL_GameState>())
			{
				GameState->AddTeamKill(
					KillerChampion->StateComponent->HasStatusTag(LOLTags::Team_Blue));
				GameState->NotifyChampionKill(KillerChampion, DeadChampion);
			}

			const float Now = GetWorld()->GetTimeSeconds();
			for (const TPair<TWeakObjectPtr<AController>, float>& Contributor : RecentDamageContributors)
			{
				AController* AssistController = Contributor.Key.Get();
				if (!AssistController ||
					AssistController == KillerInstigator ||
					Now - Contributor.Value > AssistWindowSeconds)
				{
					continue;
				}

				ABaseChampion* AssistChampion = Cast<ABaseChampion>(AssistController->GetPawn());
				if (!AssistChampion ||
					!AssistChampion->StateComponent ||
					KillerChampion->StateComponent->IsEnemy(AssistChampion->StateComponent))
				{
					continue;
				}

				if (ALOL_PlayerState* AssistPlayerState =
					AssistController->GetPlayerState<ALOL_PlayerState>())
				{
					AssistPlayerState->AddAssist();
				}
				AssistChampion->AddAssistCount();
			}
		}
	}
	else if (Cast<ABaseMinion>(OwnerPawn))
	{
		if (KillerPlayerState)
		{
			KillerPlayerState->AddMinionKill();
		}
		if (KillerChampion)
		{
			KillerChampion->AddMinionKillCount();
		}
	}
	else if (ABaseJungleMonster* DeadJungleMonster =
		Cast<ABaseJungleMonster>(OwnerPawn))
	{
		constexpr int32 JungleMonsterCS = 4;
		if (KillerPlayerState)
		{
			KillerPlayerState->AddMinionKill(JungleMonsterCS);
		}
		if (KillerChampion)
		{
			KillerChampion->AddMinionKillCount(JungleMonsterCS);
		}

		const FName MonsterName =
			DeadJungleMonster->GetJungleMonsterName();
		const bool bIsAtakhan =
			MonsterName == FName("Atakhan") ||
			MonsterName == FName("atakhan") ||
			MonsterName == FName("Atakan") ||
			MonsterName == FName("atakan");
		const bool bIsBaron =
			MonsterName == FName("Baron") ||
			MonsterName == FName("baron") ||
			MonsterName == FName("BaronNashor") ||
			MonsterName == FName("Baron_Nashor") ||
			MonsterName == FName("baron_nashor");

		if ((bIsAtakhan || bIsBaron) && KillerChampion)
		{
			TArray<AActor*> Champions;
			UGameplayStatics::GetAllActorsOfClass(
				GetWorld(),
				ABaseChampion::StaticClass(),
				Champions
			);

			for (AActor* ChampionActor : Champions)
			{
				ABaseChampion* TeamChampion =
					Cast<ABaseChampion>(ChampionActor);
				if (!TeamChampion ||
					KillerChampion->IsEnemyActor(TeamChampion) ||
					!TeamChampion->StatComponent)
				{
					continue;
				}

				if (bIsAtakhan)
				{
					TeamChampion->StatComponent
						->ApplyPermanentCombatStatBonus(
							10.0f,
							10.0f,
							5.0f,
							5.0f
						);
				}

				if (bIsBaron)
				{
					TeamChampion->StatComponent
						->ApplyTimedOffensiveStatBonus(
							30.0f,
							30.0f,
							120.0f
						);
				}
			}
		}
	}

	RecentDamageContributors.Reset();

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
		if (ABaseJungleMonster* JungleMonster = Cast<ABaseJungleMonster>(OwnerPawn))
		{
			if (ALOL_GameModeBase* GM = Cast<ALOL_GameModeBase>(GetWorld()->GetAuthGameMode()))
			{
				GM->RequestJungleMonsterRespawn(
					JungleMonster->GetJungleMonsterName(),
					JungleMonster->GetSpawnLocation(),
					JungleMonster->GetSpawnRotation(),
					JungleRespawnDelay
				);
			}
		}

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
		AttackComp->SetCombatTarget(nullptr);
		AttackComp->HitTarget = nullptr;
	}
	if (UPawnMovementComponent* MovementComp = OwnerPawn->GetMovementComponent())
	{
		MovementComp->StopMovementImmediately();
	}

	TArray<UPrimitiveComponent*> PrimitiveComps;
	OwnerPawn->GetComponents<UPrimitiveComponent>(PrimitiveComps);
	for (UPrimitiveComponent* Comp : PrimitiveComps)
	{
		if (!InitialCollisionStates.Contains(Comp))
		{
			InitialCollisionStates.Add(Comp, Comp->GetCollisionEnabled());
		}
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

	bool bIsRedTeam = false;
	if (ABaseChampion* OwnerChampion = Cast<ABaseChampion>(OwnerPawn))
	{
		bIsRedTeam =
			OwnerChampion->TeamId == 1 ||
			(OwnerChampion->StateComponent &&
			 OwnerChampion->StateComponent->HasStatusTag(LOLTags::Team_Red));
	}

	const FName TeamTag = bIsRedTeam ? FName("RedTeam") : FName("BlueTeam");
	const FName TeamAlias = bIsRedTeam ? FName("Red") : FName("Blue");
	const FName PlayerStartAlias =
		bIsRedTeam ? FName("PlayerStart2") : FName("PlayerStart1");

	for (AActor* Actor : FoundActors)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
		if (!PlayerStart)
		{
			continue;
		}

		if (PlayerStart->PlayerStartTag == TeamTag ||
			PlayerStart->PlayerStartTag == TeamAlias ||
			PlayerStart->PlayerStartTag == PlayerStartAlias ||
			PlayerStart->ActorHasTag(TeamTag) ||
			PlayerStart->ActorHasTag(TeamAlias) ||
			PlayerStart->ActorHasTag(PlayerStartAlias))
		{
			TargetStart = PlayerStart;
			break;
		}
	}

	if (TargetStart)
	{
		OwnerPawn->TeleportTo(TargetStart->GetActorLocation(), TargetStart->GetActorRotation());
	}
	else
	{
		OwnerPawn->TeleportTo(InitialSpawnLocation, InitialSpawnRotation);
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

	if (ULOL_AttackComponent* AttackComp =
		OwnerPawn->FindComponentByClass<ULOL_AttackComponent>())
	{
		AttackComp->ResetAfterRespawn();
	}

	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, ECollisionEnabled::Type>& CollisionState
		: InitialCollisionStates)
	{
		if (UPrimitiveComponent* Comp = CollisionState.Key.Get())
		{
			Comp->SetCollisionEnabled(CollisionState.Value);
		}
	}

	if (ULOL_UIComponent* UIComp = OwnerPawn->FindComponentByClass<ULOL_UIComponent>())
	{
		UIComp->GetActorWidget()->SetVisibility(true);

	}
}
