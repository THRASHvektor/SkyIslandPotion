// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogMacros.h"
#include "SIPPawn.generated.h"

/**
 * 轻量级 Pawn 基类，用于可收集物、可交互道具等简单世界实体。
 * 这些对象虽然不是 Character，但仍然能受益于 Pawn 的归属和生命周期管理。
 */
UCLASS(config=Game)
class ASIPPawn : public APawn
{
	GENERATED_BODY()

public:
	// 创建最小化 Pawn，并提供一个 SceneRoot 方便子类继续挂载组件。
	ASIPPawn(const FObjectInitializer& ObjectInitializer);

protected:
	// 标准启动入口，给派生 Pawn 保留稳定的原生扩展点。
	virtual void BeginPlay();

	// 组件初始化完成后的入口，此时子类可以假定默认子对象都已存在。
	virtual void PostInitializeComponents() override;
};
