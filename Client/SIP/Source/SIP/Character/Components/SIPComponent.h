// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "SIPComponent.generated.h"

/**
 * SIP 项目内各类自定义组件的轻量原生基类。
 * 目前主要统一构造和 BeginPlay 生命周期，后续也方便沉淀共用辅助能力。
 */
UCLASS(config=Game)
class USIPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 所有 SIP 自定义组件共用的基础构造函数。
	USIPComponent(const FObjectInitializer& ObjectInitializer);
	
protected:
	// 转发 BeginPlay，给派生 SIP 组件保留统一且稳定的生命周期入口。
	virtual void BeginPlay() override;
};
