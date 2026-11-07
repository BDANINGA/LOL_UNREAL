#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace FizzFinAsset
{
static void Build()
{
    const FString Path=TEXT("/Game/VFX/Fizz/SM_FizzRFin");
    UPackage* Package=CreatePackage(*Path); Package->FullyLoad();
    auto* Mesh=FindObject<UStaticMesh>(Package,TEXT("SM_FizzRFin"));
    if (!Mesh) Mesh=NewObject<UStaticMesh>(Package,TEXT("SM_FizzRFin"),RF_Public|RF_Standalone);
    FMeshDescription Description; FStaticMeshAttributes Attr(Description); Attr.Register();
    auto Positions=Attr.GetVertexPositions(); auto UVs=Attr.GetVertexInstanceUVs(); UVs.SetNumChannels(1);
    auto Normals=Attr.GetVertexInstanceNormals(); auto Tangents=Attr.GetVertexInstanceTangents();
    auto Signs=Attr.GetVertexInstanceBinormalSigns(); auto Colors=Attr.GetVertexInstanceColors();
    const FPolygonGroupID Group=Description.CreatePolygonGroup();
    Attr.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Fin");
    // Forward is +X. Curved leading edge, swept tip and concave trailing edge.
    const TArray<FVector3f> Outline={{68,0,0},{49,0,30},{30,0,62},{10,0,94},{-17,0,126},
        {-36,0,139},{-31,0,112},{-22,0,79},{-24,0,49},{-43,0,23},{-78,0,0}};
    auto Triangle=[&](FVector3f A,FVector3f B,FVector3f C)
    {
        const FVector3f Normal=FVector3f::CrossProduct(B-A,C-A).GetSafeNormal();
        TArray<FVertexInstanceID> Instances;
        for (FVector3f P : {A,B,C})
        {
            auto V=Description.CreateVertex(); Positions[V]=P;
            auto I=Description.CreateVertexInstance(V); Instances.Add(I);
            Normals[I]=Normal; Tangents[I]=FVector3f(1,0,0); Signs[I]=1; Colors[I]=FVector4f(1,1,1,1);
            UVs.Set(I,0,FVector2f((P.X+80.f)/160.f,P.Z/140.f));
        }
        Description.CreatePolygon(Group,Instances);
    };
    for (int32 i=0;i<Outline.Num();++i)
    {
        const auto A=Outline[i], B=Outline[(i+1)%Outline.Num()];
        // Raised central ridge gives a readable dimensional silhouette from either camera side.
        Triangle(A,B,FVector3f(-4,-15,34));
        Triangle(B,A,FVector3f(-4,15,34));
    }
    Mesh->GetStaticMaterials().Reset();
    Mesh->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/VFX/Fizz/M_FizzRFin.M_FizzRFin")),TEXT("Fin")));
    Mesh->SetNumSourceModels(1);
    Mesh->CreateMeshDescription(0,MoveTemp(Description)); Mesh->CommitMeshDescription(0);
    Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals=false;
    Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents=true;
    Mesh->Build(false); Mesh->PostEditChange(); Package->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    const FString File=FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension());
    UE_LOG(LogTemp,Display,TEXT("Fizz fin saved: %d"),UPackage::SavePackage(Package,Mesh,*File,Args));
}
static FAutoConsoleCommand Command(TEXT("LOL.Fizz.BuildRFin"),TEXT("Build the swept Fizz R fin mesh."),FConsoleCommandDelegate::CreateStatic(&Build));
}
#endif
