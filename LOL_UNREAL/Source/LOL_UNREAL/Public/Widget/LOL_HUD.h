// HUD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LOL_HUD.generated.h"

UCLASS()
class LOL_UNREAL_API ALOL_HUD : public AHUD
{
	GENERATED_BODY()
	
public:
    ALOL_HUD();
    
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> MainHUDClass;

    // 생성된 위젯 인스턴스 저장
    UPROPERTY()
    UUserWidget* MainHUDWidget;

protected:
    virtual void BeginPlay() override;
};
