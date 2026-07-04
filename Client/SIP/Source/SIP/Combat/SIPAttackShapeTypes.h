#pragma once

#include "CoreMinimal.h"
#include "CollisionShape.h"
#include "UObject/Object.h"
#include "SIPAttackShapeTypes.generated.h"

class AActor;
class UWorld;

UENUM(BlueprintType)
enum class ESIPAttackShapeType : uint8
{
	Sphere,
	Box,
	Capsule
};

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class SIP_API USIPAttackShapeSpec : public UObject
{
	GENERATED_BODY()

public:
	USIPAttackShapeSpec();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shape")
	ESIPAttackShapeType ShapeType = ESIPAttackShapeType::Sphere;

	// Local-space offset from the source actor. X is forward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	FVector LocalOffset = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebug = false;

	FTransform MakeWorldTransform(const AActor* SourceActor) const;

	virtual FCollisionShape MakeCollisionShape() const PURE_VIRTUAL(USIPAttackShapeSpec::MakeCollisionShape, return FCollisionShape::MakeSphere(1.0f););
	virtual void DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const PURE_VIRTUAL(USIPAttackShapeSpec::DrawDebugShape, );
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SIP_API USIPAttackSphereShapeSpec : public USIPAttackShapeSpec
{
	GENERATED_BODY()

public:
	USIPAttackSphereShapeSpec();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float Radius = 100.0f;

	virtual FCollisionShape MakeCollisionShape() const override;
	virtual void DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SIP_API USIPAttackBoxShapeSpec : public USIPAttackShapeSpec
{
	GENERATED_BODY()

public:
	USIPAttackBoxShapeSpec();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	FVector BoxExtent = FVector(100.0f, 80.0f, 80.0f);

	virtual FCollisionShape MakeCollisionShape() const override;
	virtual void DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const override;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SIP_API USIPAttackCapsuleShapeSpec : public USIPAttackShapeSpec
{
	GENERATED_BODY()

public:
	USIPAttackCapsuleShapeSpec();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float Radius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "1.0"))
	float HalfHeight = 100.0f;

	virtual FCollisionShape MakeCollisionShape() const override;
	virtual void DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const override;
};
