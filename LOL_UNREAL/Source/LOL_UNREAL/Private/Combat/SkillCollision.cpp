#include "Combat/SkillCollision.h"

#include "Component/LOL_StateComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

bool LOLSkillCollision::Sweep(UWorld* World, TArray<FHitResult>& Hits,
    const FVector& Start, const FVector& End, float Radius, const FCollisionQueryParams& Params)
{
    Hits.Reset();
    if (!World) return false;

    const FVector2D Delta(End.X - Start.X, End.Y - Start.Y);
    const double LengthSquared = Delta.SizeSquared();
    const double Length = FMath::Sqrt(LengthSquared);
    // Combat pawns (champions, minions, monsters and buildings) have a capsule root.
    // Iterate bodies so no finite vertical query extent can miss a different floor.
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* Target = *It;
        if (!IsValid(Target) || Params.GetIgnoredSourceObjects().Contains(Target->GetUniqueID())) continue;
        UCapsuleComponent* Body = Cast<UCapsuleComponent>(Target->GetRootComponent());
        const ULOL_StateComponent* State = Target->FindComponentByClass<ULOL_StateComponent>();
        if (!Body || !Body->IsQueryCollisionEnabled() || !State ||
            State->HasStatusTag(LOLTags::State_Dead) ||
            Params.GetIgnoredComponents().Contains(Body->GetUniqueID())) continue;

        const FVector Center = Body->GetComponentLocation();
        const FVector2D Offset(Start.X - Center.X, Start.Y - Center.Y);
        const double CombinedRadius = FMath::Max(Radius, 0.f) + Body->GetScaledCapsuleRadius();
        const double C = Offset.SizeSquared() - FMath::Square(CombinedRadius);
        double Time = 0.0;
        if (C > 0.0)
        {
            if (LengthSquared <= UE_DOUBLE_SMALL_NUMBER) continue;
            const double B = FVector2D::DotProduct(Offset, Delta);
            const double Discriminant = B * B - LengthSquared * C;
            if (Discriminant < 0.0) continue;
            Time = (-B - FMath::Sqrt(Discriminant)) / LengthSquared;
            if (Time < 0.0 || Time > 1.0) continue;
        }

        const FVector Location = FMath::Lerp(Start, End, Time);
        const FVector Normal = (Location - Center).GetSafeNormal2D();
        FHitResult Hit(Target, Body, Location, Normal);
        Hit.Location = Location;
        Hit.ImpactPoint = FVector(Location.X, Location.Y, Center.Z) - Normal * FMath::Max(Radius, 0.f);
        Hit.TraceStart = Start;
        Hit.TraceEnd = End;
        Hit.Time = Time;
        Hit.Distance = Time * Length;
        Hit.bStartPenetrating = C <= 0.0;
        Hits.Add(Hit);
    }
    Hits.Sort([](const FHitResult& A, const FHitResult& B)
    {
        if (A.Time != B.Time) return A.Time < B.Time;
        return A.GetActor()->GetUniqueID() < B.GetActor()->GetUniqueID();
    });
    return !Hits.IsEmpty();
}
