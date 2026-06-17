#include "Building/BaseBuilding.h"

#include "Component/LOL_StatComponent.h"
#include "Component/LOL_StateComponent.h"
#include "Component/LOL_AttackComponent.h"
#include "Component/LOL_LifeCycleComponent.h"
#include "Component/LOL_UIComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "BaseChampion.h"

ABaseBuilding::ABaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCapsuleHalfHeight(150.f);
	CapsuleComponent->SetCapsuleRadius(100.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn")); 

	StatComponent = CreateDefaultSubobject<ULOL_StatComponent>(TEXT("StatComponent"));
	StateComponent = CreateDefaultSubobject<ULOL_StateComponent>(TEXT("StateComponent"));
	AttackComponent = CreateDefaultSubobject<ULOL_AttackComponent>(TEXT("AttackComponent"));
	LifeCycleComponent = CreateDefaultSubobject<ULOL_LifeCycleComponent>(TEXT("LifeCycleComponent"));
	UIComponent = CreateDefaultSubobject<ULOL_UIComponent>(TEXT("UIComponent"));
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (StatComponent)
	{
		StatComponent->InitializeStat();
	}

	UpdateTeamVisual();
}

void ABaseBuilding::UpdateTeamVisual()
{
	if (!StateComponent || !AllyTexture || !EnemyTexture) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	ABaseChampion* LocalPlayer = Cast<ABaseChampion>(PC->GetPawn());

	if (!LocalPlayer || !LocalPlayer->StateComponent)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ABaseBuilding::UpdateTeamVisual);
		return;
	}

	bool bIsEnemy = LocalPlayer->StateComponent->IsEnemy(StateComponent);

	UTexture2D* TargetTexture = bIsEnemy ? EnemyTexture : AllyTexture;

	UMeshComponent* TargetMeshComp = nullptr;
	if (UStaticMeshComponent* StaticMesh = FindComponentByClass<UStaticMeshComponent>())
	{
		TargetMeshComp = StaticMesh;
	}
	else if (USkeletalMeshComponent* SkeletalMesh = FindComponentByClass<USkeletalMeshComponent>())
	{
		TargetMeshComp = SkeletalMesh;
	}
	if (TargetMeshComp)
	{
		UMaterialInstanceDynamic* DynamicMaterial = TargetMeshComp->CreateDynamicMaterialInstance(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetTextureParameterValue(FName("TeamTexture"), TargetTexture);
		}
	}

	if (UIComponent)
	{
		UTexture2D* TargetUITexture = bIsEnemy ? EnemyHPBarImage : AllyHPBarImage;
		UIComponent->UpdateHPBarImage(TargetUITexture);
	}
}