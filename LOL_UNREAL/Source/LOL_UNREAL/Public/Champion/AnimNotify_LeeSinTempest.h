#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_LeeSinTempest.generated.h"

/** Ground shock and rising grit at the strike frame of Lee Sin's E. */
UCLASS(meta = (DisplayName = "Lee Sin Tempest Impact"))
class LOL_UNREAL_API UAnimNotify_LeeSinTempest : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_LeeSinTempest();
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	UPROPERTY() TObjectPtr<class UStaticMesh> GroundPlane;
	UPROPERTY() TObjectPtr<class UMaterialInterface> GroundMaterial;
	UPROPERTY() TObjectPtr<class UMaterialInterface> GritMaterial;
};
