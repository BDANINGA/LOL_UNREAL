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
#include "Building/Building_Nexus.h"
#include "JungleMonster/BaseJungleMonster.h"

#include "Algo/Reverse.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

#include "Kismet/GameplayStatics.h"

ALOL_GameModeBase::ALOL_GameModeBase()
{
    PlayerControllerClass = ALOL_PlayerController::StaticClass();

    HUDClass = ALOL_HUD::StaticClass();

    GameStateClass = ALOL_GameState::StaticClass();

    PlayerStateClass = ALOL_PlayerState::StaticClass();

    bUseSeamlessTravel = true;
}  

void ALOL_GameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
}

UClass* ALOL_GameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    EChampionID SelectedChampion = EChampionID::None;
    if (InController)
    {
        if (const ALOL_PlayerState* PlayerState =
            InController->GetPlayerState<ALOL_PlayerState>())
        {
            SelectedChampion = PlayerState->SelectedChampion;
        }
    }

    UClass* ChampionClass = nullptr;
    switch (SelectedChampion)
    {
    case EChampionID::Alistar:
        ChampionClass = AChampion_Alistar::StaticClass();
        break;
    case EChampionID::Blitzcrank:
        ChampionClass = AChampion_Blitz::StaticClass();
        break;
    case EChampionID::Ezreal:
        ChampionClass = AChampion_Ezreal::StaticClass();
        break;
    case EChampionID::Fizz:
        ChampionClass = AChampion_Fizz::StaticClass();
        break;
    case EChampionID::Garen:
        ChampionClass = AChampion_Garen::StaticClass();
        break;
    case EChampionID::Gragas:
        ChampionClass = AChampion_Gragas::StaticClass();
        break;
    case EChampionID::Jax:
        ChampionClass = AChampion_Jax::StaticClass();
        break;
    case EChampionID::LeeSin:
        ChampionClass = AChampion_LeeSin::StaticClass();
        break;
    case EChampionID::Olaf:
        ChampionClass = AChampion_Tryndamere::StaticClass();
        break;
    case EChampionID::Vayne:
        ChampionClass = AChampion_Vayne::StaticClass();
        break;
    default:
        ChampionClass = AChampion_Garen::StaticClass();
        break;
    }

    if (ALOL_PlayerController* PlayerController =
        Cast<ALOL_PlayerController>(InController))
    {
        PlayerController->SetSelectedChampionClass(ChampionClass);
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Resolved selected champion. Player=%s ChampionID=%d Class=%s"),
        *GetNameSafe(InController),
        static_cast<int32>(SelectedChampion),
        *GetNameSafe(ChampionClass));

    return ChampionClass;
}

AActor* ALOL_GameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
    const ALOL_PlayerState* PlayerState = Player
        ? Player->GetPlayerState<ALOL_PlayerState>()
        : nullptr;

    const FName DesiredTeamTag =
        PlayerState && PlayerState->TeamID == 2
        ? FName("RedTeam")
        : FName("BlueTeam");

    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(DesiredTeamTag))
        {
            return *It;
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* ALOL_GameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    APawn* SpawnedPawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);

    if (ABaseChampion* Champion = Cast<ABaseChampion>(SpawnedPawn))
    {
        if (Champion->StateComponent)
        {
            const ALOL_PlayerState* PlayerState = NewPlayer
                ? NewPlayer->GetPlayerState<ALOL_PlayerState>()
                : nullptr;

            bool bIsBlueTeam = NewPlayer && NewPlayer->IsLocalController();
            if (PlayerState)
            {
                if (PlayerState->TeamID == 1)
                {
                    bIsBlueTeam = true;
                }
                else if (PlayerState->TeamID == 2)
                {
                    bIsBlueTeam = false;
                }
            }

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

            UE_LOG(
                LogTemp,
                Log,
                TEXT("Applied player team. Player=%s TeamID=%d Champion=%s Team=%s"),
                *GetNameSafe(NewPlayer),
                PlayerState ? PlayerState->TeamID : 0,
                *GetNameSafe(Champion),
                bIsBlueTeam ? TEXT("Blue") : TEXT("Red"));
        }
    }

    return SpawnedPawn;
}
void ALOL_GameModeBase::BeginPlay()
{
    Super::BeginPlay();

    const bool bLanePathsReady = InitializeMinionLanePaths();
    SpawnJungleMonsters();

    if (bLanePathsReady)
    {
        GetWorld()->GetTimerManager().SetTimer(
            MinionSpawnTimerHandle,
            this,
            &ALOL_GameModeBase::StartMinionWave,
            3.0f,
            false);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Minion waves disabled because lane paths could not be initialized."));
    }
}

bool ALOL_GameModeBase::InitializeMinionLanePaths()
{
    TArray<AActor*> NexusActors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ABuilding_Nexus::StaticClass(),
        NexusActors);

    AActor* BlueNexus = nullptr;
    for (AActor* NexusActor : NexusActors)
    {
        if (NexusActor &&
            (NexusActor->ActorHasTag(FName("BlueTeam")) ||
             NexusActor->ActorHasTag(FName("BlueNexus"))))
        {
            BlueNexus = NexusActor;
            break;
        }
    }

    if (!BlueNexus)
    {
        TArray<AActor*> BlueTeamActors;
        UGameplayStatics::GetAllActorsWithTag(
            GetWorld(),
            FName("BlueTeam"),
            BlueTeamActors);

        FVector BlueSpawnCenter = FVector::ZeroVector;
        int32 BluePlayerStartCount = 0;
        for (AActor* BlueTeamActor : BlueTeamActors)
        {
            if (Cast<APlayerStart>(BlueTeamActor))
            {
                BlueSpawnCenter += BlueTeamActor->GetActorLocation();
                ++BluePlayerStartCount;
            }
        }

        if (BluePlayerStartCount > 0)
        {
            BlueSpawnCenter /= static_cast<float>(BluePlayerStartCount);
            float ClosestDistanceSquared = TNumericLimits<float>::Max();

            for (AActor* NexusActor : NexusActors)
            {
                if (!NexusActor)
                {
                    continue;
                }

                const float DistanceSquared = FVector::DistSquared2D(
                    BlueSpawnCenter,
                    NexusActor->GetActorLocation());
                if (DistanceSquared < ClosestDistanceSquared)
                {
                    ClosestDistanceSquared = DistanceSquared;
                    BlueNexus = NexusActor;
                }
            }

            if (BlueNexus)
            {
                BlueNexus->Tags.AddUnique(FName("BlueNexus"));
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("Registered BlueNexus tag from BlueTeam PlayerStart distance. Nexus=%s"),
                    *BlueNexus->GetName());
            }
        }
    }

    if (!BlueNexus)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Blue Nexus was not found. Add the BlueTeam or BlueNexus actor tag to the blue Nexus."));
        return false;
    }

    const FVector BlueNexusLocation = BlueNexus->GetActorLocation();
    const bool bTopReady = InitializeLanePath(
        FName("TopLanePoint"),
        BlueNexusLocation,
        BlueTopLanePoints,
        RedTopLanePoints);
    const bool bMidReady = InitializeLanePath(
        FName("MidLanePoint"),
        BlueNexusLocation,
        BlueMidLanePoints,
        RedMidLanePoints);
    const bool bBotReady = InitializeLanePath(
        FName("BotLanePoint"),
        BlueNexusLocation,
        BlueBotLanePoints,
        RedBotLanePoints);

    return bTopReady && bMidReady && bBotReady;
}

bool ALOL_GameModeBase::InitializeLanePath(
    FName LaneTag,
    const FVector& BlueNexusLocation,
    TArray<AActor*>& OutBluePath,
    TArray<AActor*>& OutRedPath)
{
    OutBluePath.Reset();
    OutRedPath.Reset();
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), LaneTag, OutBluePath);

    OutBluePath.RemoveAll([](const AActor* Point)
    {
        return !IsValid(Point);
    });

    OutBluePath.Sort([BlueNexusLocation](const AActor& A, const AActor& B)
    {
        const float DistanceA = FVector::DistSquared2D(
            BlueNexusLocation,
            A.GetActorLocation());
        const float DistanceB = FVector::DistSquared2D(
            BlueNexusLocation,
            B.GetActorLocation());
        return DistanceA < DistanceB;
    });

    if (OutBluePath.Num() < 2)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("Lane path needs at least two points. Tag=%s Count=%d"),
            *LaneTag.ToString(),
            OutBluePath.Num());
        return false;
    }

    OutRedPath = OutBluePath;
    Algo::Reverse(OutRedPath);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Lane path initialized. Tag=%s Points=%d BlueStartDistance=%.0f RedStartDistance=%.0f"),
        *LaneTag.ToString(),
        OutBluePath.Num(),
        FVector::Dist2D(BlueNexusLocation, OutBluePath[0]->GetActorLocation()),
        FVector::Dist2D(BlueNexusLocation, OutRedPath[0]->GetActorLocation()));

    return true;
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

    const auto RegisterMonsterTagAndSpawn =
        [this](FName CampTag, FName MonsterRowName)
    {
        TArray<AActor*> MonsterTaggedTargets;
        UGameplayStatics::GetAllActorsWithTag(
            GetWorld(),
            MonsterRowName,
            MonsterTaggedTargets);

        if (MonsterTaggedTargets.Num() == 0)
        {
            UGameplayStatics::GetAllActorsWithTag(
                GetWorld(),
                CampTag,
                MonsterTaggedTargets);

            for (AActor* SpawnTarget : MonsterTaggedTargets)
            {
                if (SpawnTarget)
                {
                    SpawnTarget->Tags.AddUnique(MonsterRowName);
                }
            }

            if (MonsterTaggedTargets.Num() > 0)
            {
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("Registered jungle monster tag. CampTag=%s MonsterTag=%s Targets=%d"),
                    *CampTag.ToString(),
                    *MonsterRowName.ToString(),
                    MonsterTaggedTargets.Num());
            }
        }

        SpawnJungleMonsterAtTag(MonsterRowName, MonsterRowName);
    };

    RegisterMonsterTagAndSpawn(FName("camp_gromp"), FName("Gromp"));
    RegisterMonsterTagAndSpawn(FName("camp_wolf"), FName("Wolf"));
    RegisterMonsterTagAndSpawn(FName("camp_razorbeak"), FName("Razorbeak"));
    RegisterMonsterTagAndSpawn(FName("camp_red"), FName("Red"));
    RegisterMonsterTagAndSpawn(FName("camp_blue"), FName("Blue"));
    RegisterMonsterTagAndSpawn(FName("camp_krug"), FName("Krug"));
    RegisterMonsterTagAndSpawn(FName("camp_dragon"), FName("Atakhan"));
    RegisterMonsterTagAndSpawn(FName("camp_baron"), FName("Baron"));
}

void ALOL_GameModeBase::SpawnJungleMonsterAtTag(FName TargetTag, FName MonsterRowName)
{
    if (!GetWorld() || TargetTag.IsNone() || MonsterRowName.IsNone()) return;

    TArray<AActor*> SpawnTargets;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TargetTag, SpawnTargets);

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

                if (SelectedPoints.Num() < 2 ||
                    !IsValid(SelectedPoints[0]) ||
                    !IsValid(SelectedPoints[1]))
                {
                    UE_LOG(
                        LogTemp,
                        Error,
                        TEXT("Skipping minion spawn because lane path is invalid. PathIndex=%d Points=%d"),
                        i,
                        SelectedPoints.Num());
                    continue;
                }

                SpawnLocation = SelectedPoints[0]->GetActorLocation();
                SpawnRotation =
                    (SelectedPoints[1]->GetActorLocation() - SpawnLocation).Rotation();

                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                ABaseMinion* SpawnedMinion = GetWorld()->SpawnActor<ABaseMinion>(
                    MinionClass,
                    SpawnLocation,
                    SpawnRotation,
                    SpawnParams);

                if (SpawnedMinion)
                {
                    if (SpawnedMinion->StateComponent)
                    {
                        if (i < 3)
                        {
                            SpawnedMinion->StateComponent->AddStatusTag(LOLTags::Team_Blue);
                        }
                        else
                        {
                            SpawnedMinion->StateComponent->AddStatusTag(LOLTags::Team_Red);
                        }
                    }
                    
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
