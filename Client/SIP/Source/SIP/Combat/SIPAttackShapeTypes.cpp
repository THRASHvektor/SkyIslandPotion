#include "Combat/SIPAttackShapeTypes.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

USIPAttackShapeSpec::USIPAttackShapeSpec()
{
	ShapeType = ESIPAttackShapeType::Sphere;
}

FTransform USIPAttackShapeSpec::MakeWorldTransform(const AActor* SourceActor) const
{
	if (!SourceActor)
	{
		return FTransform::Identity;
	}

	const FTransform SourceTransform(SourceActor->GetActorRotation(), SourceActor->GetActorLocation());
	const FTransform LocalTransform(LocalRotation, LocalOffset);
	return LocalTransform * SourceTransform;
}

USIPAttackSphereShapeSpec::USIPAttackSphereShapeSpec()
{
	ShapeType = ESIPAttackShapeType::Sphere;
}

FCollisionShape USIPAttackSphereShapeSpec::MakeCollisionShape() const
{
	return FCollisionShape::MakeSphere(FMath::Max(Radius, 1.0f));
}

void USIPAttackSphereShapeSpec::DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const
{
	if (!World)
	{
		return;
	}

	DrawDebugSphere(World, ShapeTransform.GetLocation(), FMath::Max(Radius, 1.0f), 16, FColor::Red, false, Duration);
}

USIPAttackBoxShapeSpec::USIPAttackBoxShapeSpec()
{
	ShapeType = ESIPAttackShapeType::Box;
}

FCollisionShape USIPAttackBoxShapeSpec::MakeCollisionShape() const
{
	const FVector ClampedExtent(
		FMath::Max(FMath::Abs(BoxExtent.X), 1.0f),
		FMath::Max(FMath::Abs(BoxExtent.Y), 1.0f),
		FMath::Max(FMath::Abs(BoxExtent.Z), 1.0f));

	return FCollisionShape::MakeBox(ClampedExtent);
}

void USIPAttackBoxShapeSpec::DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const
{
	if (!World)
	{
		return;
	}

	const FVector ClampedExtent(
		FMath::Max(FMath::Abs(BoxExtent.X), 1.0f),
		FMath::Max(FMath::Abs(BoxExtent.Y), 1.0f),
		FMath::Max(FMath::Abs(BoxExtent.Z), 1.0f));

	DrawDebugBox(World, ShapeTransform.GetLocation(), ClampedExtent, ShapeTransform.GetRotation(), FColor::Red, false, Duration);
}

USIPAttackCapsuleShapeSpec::USIPAttackCapsuleShapeSpec()
{
	ShapeType = ESIPAttackShapeType::Capsule;
}

FCollisionShape USIPAttackCapsuleShapeSpec::MakeCollisionShape() const
{
	const float ClampedRadius = FMath::Max(Radius, 1.0f);
	const float ClampedHalfHeight = FMath::Max(HalfHeight, ClampedRadius);
	return FCollisionShape::MakeCapsule(ClampedRadius, ClampedHalfHeight);
}

void USIPAttackCapsuleShapeSpec::DrawDebugShape(UWorld* World, const FTransform& ShapeTransform, float Duration) const
{
	if (!World)
	{
		return;
	}

	const float ClampedRadius = FMath::Max(Radius, 1.0f);
	const float ClampedHalfHeight = FMath::Max(HalfHeight, ClampedRadius);
	DrawDebugCapsule(World, ShapeTransform.GetLocation(), ClampedHalfHeight, ClampedRadius, ShapeTransform.GetRotation(), FColor::Red, false, Duration);
}
