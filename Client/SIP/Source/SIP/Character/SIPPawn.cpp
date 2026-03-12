// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPPawn.h"
#include "Components/SceneComponent.h"

//////////////////////////////////////////////////////////////////////////
// ASIPPawn

ASIPPawn::ASIPPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// 创建角色组件
}

void ASIPPawn::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void ASIPPawn::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}