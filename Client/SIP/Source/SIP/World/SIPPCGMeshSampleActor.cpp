// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPPCGMeshSampleActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "PCGComponent.h"
#include "UObject/ConstructorHelpers.h"

ASIPPCGMeshSampleActor::ASIPPCGMeshSampleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SampleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SampleMesh"));
	SetRootComponent(SampleMesh);
	SampleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SampleMesh->SetWorldScale3D(FVector(16.0, 10.0, 0.08));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SampleMesh->SetStaticMesh(CubeMesh.Object);
	}

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}
