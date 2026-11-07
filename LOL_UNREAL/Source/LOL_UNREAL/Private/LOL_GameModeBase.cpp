// 게임 모드
#include "LOL_GameModeBase.h"
#include "LOL_PlayerController.h"
#include "LOL_HUD.h"
#include "LOL_GameState.h"
#include "LOL_PlayerState.h"

#include "VisionManager/VisionManager.h"

#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_VisionComponent.h"

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

    return ChampionClass;
}

AActor* ALOL_GameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
    const ALOL_PlayerState* PlayerState = Player
        ? Player->GetPlayerState<ALOL_PlayerState>()
        : nullptr;

    const uint8 TeamID = PlayerState ? PlayerState->TeamID : 0;
    const bool bIsRedTeam = TeamID == 2;
    const FName DesiredTeamTag = bIsRedTeam
        ? FName("RedTeam")
        : FName("BlueTeam");
    const FName DesiredTeamAlias = bIsRedTeam
        ? FName("Red")
        : FName("Blue");
    const FName DesiredPlayerStartAlias = bIsRedTeam
        ? FName("PlayerStart2")
        : FName("PlayerStart1");

    // PlayerStartTag is the value shown as "Player Start Tag" in the editor.
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        if (It->PlayerStartTag == DesiredTeamTag ||
            It->PlayerStartTag == DesiredTeamAlias ||
            It->PlayerStartTag == DesiredPlayerStartAlias)
        {
            return *It;
        }
    }

    // Preserve compatibility with older maps that used Actor Tags.
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(DesiredTeamTag) ||
            It->ActorHasTag(DesiredTeamAlias) ||
            It->ActorHasTag(DesiredPlayerStartAlias))
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
            if (Champion->VisionComponent)
            {
                AVisionManager* VisionManager =
                    Cast<AVisionManager>(
                        UGameplayStatics::GetActorOfClass(
                            GetWorld(),
                            AVisionManager::StaticClass()
                        )
                    );

                if (VisionManager)
                {
                    VisionManager->RegisterVisionComponent(Champion->VisionComponent);
                }
            }
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
            }
        }
    }

    if (!BlueNexus)
    {
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
        return false;
    }

    OutRedPath = OutBluePath;
    Algo::Reverse(OutRedPath);

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

void ALOL_GameModeBase::NotifyNexusDestroyed(const ABuilding_Nexus* Nexus)
{
    if (!HasAuthority() || !IsValid(Nexus)) return;
    ALOL_GameState* GS = GetWorld()->GetGameState<ALOL_GameState>();
    if (!GS || GS->HasMatchFinished()) return;

    const ULOL_StateComponent* State = Nexus->FindComponentByClass<ULOL_StateComponent>();
    bool bBlueNexus = State && State->HasStatusTag(LOLTags::Team_Blue);
    bool bRedNexus = State && State->HasStatusTag(LOLTags::Team_Red);
    if (!bBlueNexus && !bRedNexus)
    {
        bBlueNexus = Nexus->ActorHasTag(TEXT("BlueTeam")) || Nexus->ActorHasTag(TEXT("BlueNexus"));
        bRedNexus = Nexus->ActorHasTag(TEXT("RedTeam")) || Nexus->ActorHasTag(TEXT("RedNexus"));
    }
    if (bBlueNexus == bRedNexus)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot resolve team for destroyed nexus %s"), *Nexus->GetName());
        return;
    }

    if (GS->FinishMatch(bBlueNexus ? 2 : 1))
    {
        GetWorldTimerManager().ClearTimer(MinionSpawnTimerHandle);
    }
}

void ALOL_GameModeBase::NotifyInhibitorDestroyed(const ABuilding_Inhibitor* Inhibitor)
{
    if (!HasAuthority() || !IsValid(Inhibitor)) return;

    const ULOL_StateComponent* State = Inhibitor->FindComponentByClass<ULOL_StateComponent>();
    bool bBlueInhibitor = State && State->HasStatusTag(LOLTags::Team_Blue);
    bool bRedInhibitor = State && State->HasStatusTag(LOLTags::Team_Red);
    if (!bBlueInhibitor && !bRedInhibitor)
    {
        bBlueInhibitor = Inhibitor->ActorHasTag(TEXT("BlueTeam"));
        bRedInhibitor = Inhibitor->ActorHasTag(TEXT("RedTeam"));
    }
    if (bBlueInhibitor == bRedInhibitor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot resolve team for destroyed inhibitor %s"), *Inhibitor->GetName());
        return;
    }

    int32 LaneIndex = INDEX_NONE;
    const FName LaneTags[] = {TEXT("Top"), TEXT("Mid"), TEXT("Bot")};
    const FName LaneAliases[] = {TEXT("TopLane"), TEXT("MidLane"), TEXT("BotLane")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (Inhibitor->ActorHasTag(LaneTags[Index]) || Inhibitor->ActorHasTag(LaneAliases[Index]))
        {
            LaneIndex = Index;
            break;
        }
    }

    // Existing maps can omit lane tags; use the nearest lane segment in the ground plane.
    if (LaneIndex == INDEX_NONE)
    {
        const TArray<AActor*>* Paths[] = {&BlueTopLanePoints, &BlueMidLanePoints, &BlueBotLanePoints};
        FVector Location = Inhibitor->GetActorLocation();
        Location.Z = 0.f;
        float ClosestDistanceSquared = TNumericLimits<float>::Max();
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const TArray<AActor*>& Path = *Paths[Index];
            for (int32 PointIndex = 1; PointIndex < Path.Num(); ++PointIndex)
            {
                if (!IsValid(Path[PointIndex - 1]) || !IsValid(Path[PointIndex])) continue;
                FVector Start = Path[PointIndex - 1]->GetActorLocation();
                FVector End = Path[PointIndex]->GetActorLocation();
                Start.Z = End.Z = 0.f;
                const float DistanceSquared = FVector::DistSquared(
                    Location, FMath::ClosestPointOnSegment(Location, Start, End));
                if (DistanceSquared < ClosestDistanceSquared)
                {
                    ClosestDistanceSquared = DistanceSquared;
                    LaneIndex = Index;
                }
            }
        }
    }
    if (LaneIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot resolve lane for destroyed inhibitor %s"), *Inhibitor->GetName());
        return;
    }

    // Destroying a blue inhibitor empowers red minions, and vice versa.
    SuperMinionLanes.Add(LaneIndex + (bBlueInhibitor ? 3 : 0));
}

void ALOL_GameModeBase::StartMinionWave()
{
    const ALOL_GameState* GS = GetWorld()->GetGameState<ALOL_GameState>();
    if (GS && GS->HasMatchFinished()) return;
    const float MatchTime = GS ? GS->CurrentMatchTime : 0.f;
    int32 TimeWave = 0;
    if (MatchTime < 15.0f * 60.0f) {
        TimeWave = 3;
    }
    else if (MatchTime < 25.0f * 60.0f) {
        TimeWave = 2;
    }
    else
        TimeWave = 1;
    // Snapshot once so an inhibitor destroyed mid-wave affects the next wave only.
    CurrentWaveSuperMinionLanes = SuperMinionLanes;
    bCurrentWaveHasSiegeMinion = (MinionWaveCount + 1) % TimeWave == 0;

    CurrentWaveMinions.Empty();
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    CurrentWaveMinions.Add(AMinion_Melee::StaticClass());
    // Reserve this slot even on non-siege waves so empowered lanes always get a super minion.
    CurrentWaveMinions.Add(AMinion_Siege::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());
    CurrentWaveMinions.Add(AMinion_Caster::StaticClass());

    SpawnedMinionCount = 0;
    MinionWaveCount += 1;

    GetWorld()->GetTimerManager().SetTimer(MinionSpawnTimerHandle, this, &ALOL_GameModeBase::SpawnNextMinion, 1.0f, true, 0.0f);
}

void ALOL_GameModeBase::SpawnNextMinion()
{
    const ALOL_GameState* GS = GetWorld()->GetGameState<ALOL_GameState>();
    if (GS && GS->HasMatchFinished()) return;
    TArray<AActor*> SelectedPoints{};
    FVector SpawnLocation = FVector::ZeroVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;
    if (SpawnedMinionCount < CurrentWaveMinions.Num())
    {
        UClass* WaveMinionClass = CurrentWaveMinions[SpawnedMinionCount];
        if (WaveMinionClass)
        {
            for (int i = 0; i < 6; ++i) {
                UClass* MinionClass = WaveMinionClass;
                if (WaveMinionClass == AMinion_Siege::StaticClass())
                {
                    if (CurrentWaveSuperMinionLanes.Contains(i))
                    {
                        MinionClass = AMinion_Super::StaticClass();
                    }
                    else if (!bCurrentWaveHasSiegeMinion)
                    {
                        continue;
                    }
                }
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

                    if (AVisionManager* VisionManager =
                        Cast<AVisionManager>(
                            UGameplayStatics::GetActorOfClass(
                                GetWorld(),
                                AVisionManager::StaticClass())))
                    {
                        if (ULOL_VisionComponent* VisionComp =
                            SpawnedMinion->FindComponentByClass<ULOL_VisionComponent>())
                        {
                            VisionManager->RegisterVisionComponent(VisionComp);
                        }

                        VisionManager->RegisterActor(SpawnedMinion);
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
