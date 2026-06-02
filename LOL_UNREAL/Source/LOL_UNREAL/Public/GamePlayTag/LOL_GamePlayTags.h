#pragma once
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "NativeGameplayTags.h"

namespace LOLTags
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Champion_Ranged);

    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Moving);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Silenced);

    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_Q);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_W);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_E);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill_R);
}
