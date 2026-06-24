// 상태 컴포넌트 (게임플레이 태그)
#include "Component/LOL_StateComponent.h"
#include "Net/UnrealNetwork.h"

ULOL_StateComponent::ULOL_StateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULOL_StateComponent::BeginPlay()
{
	Super::BeginPlay();
}
void ULOL_StateComponent::AddStatusTag(FGameplayTag Tag)
{
	if (GetOwnerRole() == ROLE_Authority && Tag.IsValid())
	{
		StatusTags.AddTag(Tag);
		OnStateTagsChanged.Broadcast();
	}
}

void ULOL_StateComponent::RemoveStatusTag(FGameplayTag Tag)
{
	if (GetOwnerRole() == ROLE_Authority && Tag.IsValid())
	{
		StatusTags.RemoveTag(Tag);
		OnStateTagsChanged.Broadcast();
	}
}

bool ULOL_StateComponent::HasStatusTag(FGameplayTag Tag) const
{
	return StatusTags.HasTag(Tag);
}

bool ULOL_StateComponent::IsEnemy(ULOL_StateComponent* OtherState) const
{
	if (!OtherState) return false;

	if (this == OtherState) return false;

	if (HasStatusTag(LOLTags::Team_Jungle) || OtherState->HasStatusTag(LOLTags::Team_Jungle))
	{
		return true;
	}

	if (HasStatusTag(LOLTags::Team_Blue) && OtherState->HasStatusTag(LOLTags::Team_Blue)) return false;
	if (HasStatusTag(LOLTags::Team_Red) && OtherState->HasStatusTag(LOLTags::Team_Red)) return false;

	return true;
}

void ULOL_StateComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(StatusTags);
}

void ULOL_StateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULOL_StateComponent, StatusTags);
}

void ULOL_StateComponent::OnRep_StatusTags()
{
	OnStateTagsChanged.Broadcast();
}
