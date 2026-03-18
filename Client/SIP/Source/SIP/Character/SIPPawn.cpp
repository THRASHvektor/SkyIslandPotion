// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPPawn.h"
#include "Components/SceneComponent.h"

// 先搭一个最基础的 Pawn 根结构，具体外观和行为由子类自己补充。
//////////////////////////////////////////////////////////////////////////
// SIP Pawn 基类实现

ASIPPawn::ASIPPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	// 创建默认场景根组件，方便后续附加显示或交互组件。
}

// 基础 Pawn 的启动阶段先保持默认生命周期，方便子类按需扩展。
void ASIPPawn::BeginPlay()
{
	// 先执行父类逻辑。
	Super::BeginPlay();
}

// 保留显式重写，给 SIP 的 Pawn 派生类一个稳定的原生扩展点。
void ASIPPawn::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}
