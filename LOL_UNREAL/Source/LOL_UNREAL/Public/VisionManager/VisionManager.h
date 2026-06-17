#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

//class ABaseChampion;
//class UCanvas;
//class UCanvasRenderTarget2D;
//class UTexture2D;
//class ULOL_StateComponent;
//
//UCLASS()
//class LOL_UNREAL_API AVisionManager : public AActor
//{
//	GENERATED_BODY()
//
//public:
//	AVisionManager();
//
//	virtual void BeginPlay() override;
//	virtual void Tick(float DeltaTime) override;
//
//	UFUNCTION(BlueprintCallable, Category = "Vision")
//	UCanvasRenderTarget2D* GetVisionRenderTarget() const { return VisionRenderTarget; }
//
//protected:
//	UFUNCTION()
//	void DrawVisionMask(UCanvas* Canvas, int32 Width, int32 Height);
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
//	FVector2D MapCenter = FVector2D::ZeroVector;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Map")
//	FVector2D MapWorldSize = FVector2D(16000.0f, 16000.0f);
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|RenderTarget", meta = (ClampMin = "64", ClampMax = "4096"))
//	int32 RenderTargetSize = 512;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|RenderTarget")
//	float HiddenFowValue = 0.4f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|RenderTarget")
//	float VisibleFowValue = 1.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|LineOfSight", meta = (ClampMin = "8", ClampMax = "720"))
//	int32 VisionTraceSegments = 256;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|LineOfSight")
//	float VisionTraceHeight = 80.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|LineOfSight")
//	float VisionBlockerProbeBackoff = 8.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Sources")
//	bool bUseChampionVision = true;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Sources")
//	bool bUseMinionVision = true;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Visibility")
//	bool bHideEnemiesOutsideVision = true;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Material")
//	bool bApplyRenderTargetToWorldMaterials = true;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Material")
//	TArray<FName> VisionTextureParameterNames;
//
//private:
//	struct FVisionSource
//	{
//		FVector Location = FVector::ZeroVector;
//		float Radius = 0.0f;
//	};
//
//	UPROPERTY(Transient)
//	TObjectPtr<UCanvasRenderTarget2D> VisionRenderTarget;
//
//	UPROPERTY(Transient)
//	TObjectPtr<UTexture2D> CanvasWhiteTexture;
//
//	UPROPERTY(Transient)
//	TObjectPtr<ABaseChampion> LocalChampion;
//
//	TArray<FVisionSource> CachedVisionSources;
//
//	void RefreshVision();
//	void CollectVisionSources();
//	void UpdateActorVisibility();
//	void ApplyRenderTargetToWorldMaterials();
//
//	ABaseChampion* ResolveLocalChampion() const;
//	ULOL_StateComponent* GetLocalState() const;
//	bool IsFriendlyVisionSource(AActor* Actor, ULOL_StateComponent* LocalState) const;
//	bool ShouldControlVisibility(AActor* Actor, ULOL_StateComponent* LocalState) const;
//	bool IsWorldLocationVisible(const FVector& WorldLocation) const;
//	bool HasLineOfSight(const FVector& From, const FVector& To) const;
//	FVector2D WorldToCanvas(const FVector& WorldLocation, int32 Width, int32 Height) const;
//	FVector CanvasToWorld2D(const FVector2D& CanvasLocation, int32 Width, int32 Height, float Z) const;
//	void AddTriangle(TArray<FCanvasUVTri>& Triangles, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color) const;
//	void AddVisionSourceTriangles(TArray<FCanvasUVTri>& Triangles, const FVisionSource& Source, int32 Width, int32 Height) const;
//};
