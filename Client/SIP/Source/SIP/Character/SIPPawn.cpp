// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPPawn.h"


//////////////////////////////////////////////////////////////////////////
// ASIPPawn

ASIPPawn::ASIPPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 创建角色组件
	CreateComponent();
}

void ASIPPawn::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void ASIPPawn::CreateComponent()
{
    // 在这里创建Pawn需要的组件
}

void ASIPPawn::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}