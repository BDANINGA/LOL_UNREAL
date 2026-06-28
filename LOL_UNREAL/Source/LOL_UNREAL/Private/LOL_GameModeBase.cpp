// 게임 모드
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"
#include "LOL_GameState.h"
#include "LOL_PlayerState.h"

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

#include "Minion/Minion_Melee.h"
#include "Minion/Minion_Caster.h"
#include "Minion/Minion_Siege.h"
#include "Minion/Minion_Super.h"

#include "Building/Building_Inhibitor.h"
#include "JungleMonster/BaseJungleMonster.h"

#include "EngineUtils.h"
#include "TimerManager.h"

#include "Kismet/GameplayStatics.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();

    HUDClass = ALOL_HUD::StaticClass();

    GameStateClass = ALOL_GameState::StaticClass();

    PlayerStateClass = ALOL_PlayerState::StaticClass();
}  

UClass* ALOL_GameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    // 1. 해당 컨트롤러가 '로컬'에서 실행되는 서버 컨트롤러인지 확인
    // 리슨 서버 모드에서 0번 플레이어(방장)는 IsLocalController()가 true이며, 서버 권한을 가집니다.
    if (InController && InController->IsLocalController())
    {
        // 첫 번째 플레이어
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
            const bool bForceRedForTryndamere = Champion->IsA(AChampion_Tryndamere::StaticClass());
            const bool bIsBlueTeam = !bForceRedForTryndamere && NewPlayer && NewPlayer->IsLocalController();

            Champion->StateComponent->RemoveStatusTag(LOLTags::Team_Blue);
            Champion->StateComponent->RemoveStatusTag(LOLTags::Team_Red);

            if (bIsBlueTeam)
            {
                Champion->TeamId = 0;
                Champion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
            }
            else
            {
                Champion->TeamId = 1;
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
    SpawnJungleMonsters();

    GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::StartMinionWave, 3.0f, false);
}

void ALOL_GameModeBase::RequestRespawn(ABaseChampion* DeadChampion)
{
    if (!DeadChampion) return;

    int32 ChampionLevel = 1;
    ULOL_StatComponent* StatComp = DeadChampion->FindComponentByClass<ULOL_StatComponent>();
    if (StatComp)
    {
        ChampionLevel = StatComp->GetStat().Level;
    }

    float RespawnDelay = 10.0f + (ChampionLevel * 2.5f);

    ULOL_LifeCycleComponent* LifeCycleComp = DeadChampion->FindComponentByClass<ULOL_LifeCycleComponent>();

    if (LifeCycleComp)
    {
        FTimerHandle RespawnTimer;
        FTimerDelegate RespawnDelegate;

        RespawnDelegate.BindUObject(LifeCycleComp, &ULOL_LifeCycleComponent::Respawn);

        GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
    }
}

void ALOL_GameModeBase::RequestJungleMonsterRespawn(FName MonsterRowName, FVector SpawnLocation, FRotator SpawnRotation, float RespawnDelay)
{
    if (!HasAuthority() || MonsterRowName.IsNone()) return;

    FTimerHandle RespawnTimerHandle;
    FTimerDelegate RespawnDelegate;
    RespawnDelegate.BindUObject(
        this,
        &ALOL_GameModeBase::SpawnJungleMonsterAtTransform,
        MonsterRowName,
        SpawnLocation,
        SpawnRotation
    );

    GetWorldTimerManager().SetTimer(
        RespawnTimerHandle,
        RespawnDelegate,
        FMath::Max(0.1f, RespawnDelay),
        false
    );
}

void ALOL_GameModeBase::SpawnJungleMonsters()
{
    if (!HasAuthority()) return;

    SpawnJungleMonsterAtTag(FName("gromp_target"), FName("Gromp"));
    SpawnJungleMonsterAtTag(FName("wolf_target"), FName("Wolf"));
    SpawnJungleMonsterAtTag(FName("Razorbeak_target"), FName("Razorbeak"));
    SpawnJungleMonsterAtTag(FName("red_target"), FName("Red"));
    SpawnJungleMonsterAtTag(FName("blue_target"), FName("Blue"));
    SpawnJungleMonsterAtTag(FName("krug_target"), FName("Krug"));
    SpawnJungleMonsterAtTag(FName("dragon_target"), FName("Atakhan"));
    SpawnJungleMonsterAtTag(FName("baron_target"), FName("Baron"));
}

void ALOL_GameModeBase::SpawnJungleMonsterAtTag(FName TargetTag, FName MonsterRowName)
{
    if (!GetWorld() || TargetTag.IsNone() || MonsterRowName.IsNone()) return;

    TArray<AActor*> SpawnTargets;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TargetTag, SpawnTargets);

    if (SpawnTargets.Num() == 0)
    {
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

        TArray<FString> TargetNames;
        const FString TargetTagString = TargetTag.ToString();
        FString BaseTargetName = TargetTagString;
        BaseTargetName.RemoveFromEnd(TEXT("_target"), ESearchCase::IgnoreCase);

        TargetNames.Add(TargetTagString);

        if (BaseTargetName.Equals(TEXT("atakhan"), ESearchCase::IgnoreCase) ||
            BaseTargetName.Equals(TEXT("dragon"), ESearchCase::IgnoreCase))
        {
            TargetNames.Add(TEXT("dragon"));
            TargetNames.Add(TEXT("dragon_target"));
            TargetNames.Add(TEXT("dragon_spawn"));
            TargetNames.Add(TEXT("dragonTarget"));
            TargetNames.Add(TEXT("dragonSpawn"));
            TargetNames.Add(TEXT("atakhan"));
            TargetNames.Add(TEXT("atakhan_target"));
            TargetNames.Add(TEXT("atakhan_spawn"));
            TargetNames.Add(TEXT("atakhanTarget"));
            TargetNames.Add(TEXT("atakhanSpawn"));
            TargetNames.Add(TEXT("atakan"));
            TargetNames.Add(TEXT("atakan_target"));
            TargetNames.Add(TEXT("atakan_spawn"));
        }

        if (BaseTargetName.Equals(TEXT("baron"), ESearchCase::IgnoreCase))
        {
            TargetNames.Add(TEXT("baron"));
            TargetNames.Add(TEXT("baron_target"));
            TargetNames.Add(TEXT("baron_spawn"));
            TargetNames.Add(TEXT("baronTarget"));
            TargetNames.Add(TEXT("baronSpawn"));
            TargetNames.Add(TEXT("nashor"));
            TargetNames.Add(TEXT("nashor_target"));
            TargetNames.Add(TEXT("nashor_spawn"));
            TargetNames.Add(TEXT("baron_nashor"));
            TargetNames.Add(TEXT("baron_nashor_target"));
            TargetNames.Add(TEXT("baron_nashor_spawn"));
        }

        for (AActor* Actor : AllActors)
        {
            if (!Actor) continue;

            for (const FString& TargetName : TargetNames)
            {
                if (Actor->GetName().Contains(TargetName, ESearchCase::IgnoreCase) ||
                    Actor->GetActorLabel().Contains(TargetName, ESearchCase::IgnoreCase))
                {
                    SpawnTargets.AddUnique(Actor);
                    break;
                }
            }
        }
    }

    if (SpawnTargets.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Jungle monster spawn target not found. Tag=%s Monster=%s"),
            *TargetTag.ToString(),
            *MonsterRowName.ToString()
        );
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Red,
                FString::Printf(
                    TEXT("Jungle spawn target not found: %s"),
                    *TargetTag.ToString()
                )
            );
        }
        return;
    }

    for (AActor* SpawnTarget : SpawnTargets)
    {
        if (!SpawnTarget) continue;

        SpawnJungleMonsterAtTransform(
            MonsterRowName,
            SpawnTarget->GetActorLocation(),
            SpawnTarget->GetActorRotation()
        );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Jungle monster spawned. Monster=%s Target=%s Location=%s"),
            *MonsterRowName.ToString(),
            *SpawnTarget->GetName(),
            *SpawnTarget->GetActorLocation().ToString()
        );
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Green,
                FString::Printf(
                    TEXT("Spawned jungle monster: %s"),
                    *MonsterRowName.ToString()
                )
            );
        }
    }
}

void ALOL_GameModeBase::SpawnJungleMonsterAtTransform(FName MonsterRowName, FVector SpawnLocation, FRotator SpawnRotation)
{
    if (!GetWorld() || MonsterRowName.IsNone()) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ABaseJungleMonster* SpawnedMonster =
        GetWorld()->SpawnActor<ABaseJungleMonster>(
            ABaseJungleMonster::StaticClass(),
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

    if (!SpawnedMonster) return;

    SpawnedMonster->InitializeJungleMonster(MonsterRowName);

    if (SpawnedMonster->StateComponent)
    {
        SpawnedMonster->StateComponent->AddStatusTag(LOLTags::Team_Jungle);
    }
}

void ALOL_GameModeBase::StartMinionWave()
{
    /*TArray<AActor*> FoundBuilding;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding_Inhibitor::StaticClass(), FoundBuilding);

    AActor* TargetLine = nullptr;

    FName TeamTag = "BlueTeam";
    FName LineTag = "Top";



    for (AActor* Actor : FoundBuilding)
    {
        if (Actor->ActorHasTag(TeamTag))
        {
            TargetLine = Actor;
            break;
        }
    }*/

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
    if (MinionWaveCount != 0 || MinionWaveCount % TimeWave == 0)
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
