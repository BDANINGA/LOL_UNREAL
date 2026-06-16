// 게임 모드
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"
#include "LOL_GameState.h"

#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StateComponent.h"

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

#include "Kismet/GameplayStatics.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();

    HUDClass = ALOL_HUD::StaticClass();

    GameStateClass = ALOL_GameState::StaticClass();
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

APawn* ALOL_GameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    APawn* SpawnedPawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);

    if (ABaseChampion* Champion = Cast<ABaseChampion>(SpawnedPawn))
    {
        if (Champion->StateComponent)
        {
            if (NewPlayer && NewPlayer->IsLocalController())
            {
                Champion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
            }
            else
            {
                Champion->StateComponent->AddStatusTag(LOLTags::Team_Red);
            }
        }
    }

    return SpawnedPawn;
}

void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();

    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("TopLanePoint"), BlueTopLanePoints);
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MidLanePoint"), BlueMidLanePoints);
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("BotLanePoint"), BlueBotLanePoints);
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("TopLanePoint"), RedTopLanePoints);
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MidLanePoint"), RedMidLanePoints);
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("BotLanePoint"), RedBotLanePoints);
    
    BlueTopLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() < B.GetActorLabel();
        });
    BlueMidLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() < B.GetActorLabel();
        });
    BlueBotLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() < B.GetActorLabel();
        });
    RedTopLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() > B.GetActorLabel();
        });
    RedMidLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() > B.GetActorLabel();
        });
    RedBotLanePoints.Sort([](const AActor& A, const AActor& B) {
        return A.GetActorLabel() > B.GetActorLabel();
        });
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
    ALOL_GameState* GS = Cast<ALOL_GameState>(GetWorld()->GetGameState());
    int32 TimeWave = 0;
    if (GS->CurrentMatchTime < 15.0f * 60.0f) {
        TimeWave = 3;
    }
    else if (GS->CurrentMatchTime < 25.0f * 60.0f) {
        TimeWave = 2;
    }
    else
        TimeWave = 1;


    CurrentWaveMinions.Empty();
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    if (MinionWaveCount % TimeWave == 0)
    {
        CurrentWaveMinions.Add(AMinion_Siege::StaticClass());
    }
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());

    SpawnedMinionCount = 0;
    MinionWaveCount += 1;

    GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::SpawnNextMinion, 1.0f, true, 0.0f);
}

void ALOL_GameModeBase::SpawnNextMinion()
{
    TArray<AActor*> SelectedPoints{};
    FVector SpawnLocation = FVector::ZeroVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;
    if (SpawnedMinionCount < CurrentWaveMinions.Num())
    {
        UClass* MinionClass = CurrentWaveMinions[SpawnedMinionCount];
        if (MinionClass)
        {
            for (int i = 0; i < 6; ++i) {
                if (i == 0) SelectedPoints = BlueTopLanePoints;
                else if (i == 1) SelectedPoints = BlueMidLanePoints;
                else if (i == 2) SelectedPoints = BlueBotLanePoints;
                else if (i == 3) SelectedPoints = RedTopLanePoints;
                else if (i == 4) SelectedPoints = RedMidLanePoints;
                else if (i == 5) SelectedPoints = RedBotLanePoints;
                SpawnLocation = SelectedPoints[0]->GetActorLocation();
                SpawnRotation = SelectedPoints[0]->GetActorRotation();

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                ABaseMinion* SpawnedMinion = GetWorld()->SpawnActor<ABaseMinion>(
                    MinionClass,
                    SpawnLocation,
                    SpawnRotation,
                    SpawnParams);

                if (SpawnedMinion)
                {
                    if (i < 3) SpawnedMinion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
                    else SpawnedMinion->StateComponent->AddStatusTag(LOLTags::Team_Red);
                    
                    for (AActor* Point : SelectedPoints)
                    {
                        if (Point)
                        {
                            SpawnedMinion->PathPoints.Add(Point->GetActorLocation());
                        }
                    }

                    SpawnedMinion->MoveToNextWaypoint();
                }
            }
            SpawnedMinionCount++;
        }
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(MinionSpawnTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::StartMinionWave, 30.0f, false);
    }
}
