// Fill out your copyright notice in the Description page of Project Settings.
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"

#include "Component/LOL_LifeCycleComponent.h"

#include "Champion/Champion_Alistar.h"
#include "Champion/Champion_Vayne.h"
#include "Champion/Champion_Blitz.h"
#include "Champion/Champion_Garen.h"
#include "Champion/Champion_Ezreal.h"
#include "Champion/Champion_Fizz.h"
#include "Champion/Champion_Jax.h"
#include "Champion/Champion_Leesin.h"
#include "Champion/Champion_Gragas.h"
#include "Champion/Champion_Tryndamere.h"
#include "VisionManager/VisionManager.h"


#include "Minion/Minion_Melee.h"
#include "Minion/Minion_Caster.h"
#include "Minion/Minion_Siege.h"
#include "Minion/Minion_Super.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();
    
    // 플레이어마다 캐릭터를 다르게 설정하기 위해서는 필요하지 않음.
    // DefaultPawnClass = AChampion_Alistar::StaticClass();

    HUDClass = ALOL_HUD::StaticClass();
}  

UClass* ALOL_GameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    // 1. 해당 컨트롤러가 '로컬'에서 실행되는 서버 컨트롤러인지 확인
    // 리슨 서버 모드에서 0번 플레이어(방장)는 IsLocalController()가 true이며, 서버 권한을 가집니다.
    if (InController && InController->IsLocalController())
    {
        // 첫 번째 플레이어(방장)는 알리스타
        return AChampion_Alistar::StaticClass();
    }

    // 2. 그 외에 접속하는 클라이언트 플레이어들
    return AChampion_Tryndamere::StaticClass();
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        bool bHasVisionManager = false;
        for (TActorIterator<AVisionManager> It(GetWorld()); It; ++It)
        {
            bHasVisionManager = true;
            break;
        }

        if (!bHasVisionManager)
        {
            GetWorld()->SpawnActor<AVisionManager>(
                AVisionManager::StaticClass(),
                FVector::ZeroVector,
                FRotator::ZeroRotator
            );
        }
    }

    MinionSpawnLocation = FVector(-4803.f, 5708.f, -1201.f);
    GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::StartMinionWave, 3.0f, false);
}

void ALOL_GameModeBase::RequestRespawn(ABaseChampion* DeadChampion)
{
    float RespawnDelay = 5.0f; // 나중에는 레벨에 따라 계산식 적용

    ULOL_LifeCycleComponent* LifeCycleComp = DeadChampion->FindComponentByClass<ULOL_LifeCycleComponent>();

    if (LifeCycleComp)
    {
        FTimerHandle RespawnTimer;
        FTimerDelegate RespawnDelegate;

        // 핵심 수정: 바인딩 대상을 컴포넌트와 컴포넌트의 Respawn 함수로 변경합니다.
        RespawnDelegate.BindUObject(LifeCycleComp, &ULOL_LifeCycleComponent::Respawn);

        GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
    }
}

void ALOL_GameModeBase::StartMinionWave()
{
    // 웨이브 구성: 전사 3마리 -> 마법사 3마리 순서로 세팅
    CurrentWaveMinions.Empty();
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());

    // 대포 미니언이 필요할 때 아래 코드 활성화
    // CurrentWaveMinions.Add(AMinion_Siege::StaticClass());

    SpawnedMinionCount = 0;

    // 1초 간격으로 반복하면서 하나씩 스폰 (첫 스폰은 딜레이 없이 즉시 실행)
    GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::SpawnNextMinion, 1.0f, true, 0.0f);
}

void ALOL_GameModeBase::SpawnNextMinion()
{
    // 아직 스폰할 미니언이 남아있다면
    if (SpawnedMinionCount < CurrentWaveMinions.Num())
    {
        UClass* MinionClass = CurrentWaveMinions[SpawnedMinionCount];
        if (MinionClass)
        {
            GetWorld()->SpawnActor<ABaseMinion>(
                MinionClass,
                MinionSpawnLocation,
                FRotator::ZeroRotator
            );
        }
        SpawnedMinionCount++; // 카운트 증가
    }
    else
    {
        // 웨이브 스폰이 끝났으므로 타이머 정지
        GetWorld()->GetTimerManager().ClearTimer(MinionSpawnTimerHandle);

        GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::StartMinionWave, 30.0f, false);
    }
}
