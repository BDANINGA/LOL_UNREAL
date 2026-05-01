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
#include "Net/UnrealNetwork.h"

ULOL_LifeCycleComponent::ULOL_LifeCycleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void ULOL_LifeCycleComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<ABaseChampion>(GetOwner());
	if (Owner && Owner->StatComponent)
	{
		Owner->StatComponent->OnHpZero.AddUObject(this, &ULOL_LifeCycleComponent::Server_HandleDeath);
	}
	
}
void ULOL_LifeCycleComponent::Server_HandleDeath()
{
	if (bIsDead || !Owner->HasAuthority()) return;

	bIsDead = true;
	Multicast_OnDeath();

	if (ALOL_GameModeBase* GM = Cast<ALOL_GameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GM->RequestRespawn(Owner);
	}
}
void ULOL_LifeCycleComponent::Multicast_OnDeath_Implementation()
{
	if (Owner->MoveComponent) Owner->MoveComponent->StopMovement();
	if (Owner->AttackComponent) Owner->AttackComponent->ReceivedCrowdControl(); 

			
	Owner->GetCharacterMovement()->DisableMovement();
	Owner->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Owner->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Owner->UIComponent->GetHpBar()->SetVisibility(false);
	Owner->UIComponent->GetMpBar()->SetVisibility(false);
}
void ULOL_LifeCycleComponent::Respawn()
{
	if (!Owner->HasAuthority()) return;

	bIsDead = false;

	if (Owner->StatComponent)
	{
		Owner->StatComponent->SetHp(Owner->StatComponent->GetStat().MaxHP);
		Owner->StatComponent->SetMp(Owner->StatComponent->GetStat().MaxMP);
	}

	Owner->SetActorLocation(FVector(0, 0, 100));
	Multicast_OnRespawn();
}
void ULOL_LifeCycleComponent::Multicast_OnRespawn_Implementation()
{
	Owner->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Owner->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Owner->GetMesh()->SetCollisionResponseToAllChannels(ECR_Block);

	Owner->UIComponent->GetHpBar()->SetVisibility(true);
	Owner->UIComponent->GetMpBar()->SetVisibility(true);
	Owner->PlayAnimMontage(nullptr);
}
void ULOL_LifeCycleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULOL_LifeCycleComponent, bIsDead);
}