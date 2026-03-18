// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

// 项目内统一使用的日志分类，覆盖玩法、角色和 GAS 相关代码路径。
SIP_API DECLARE_LOG_CATEGORY_EXTERN(LogSIP, Log, All);
SIP_API DECLARE_LOG_CATEGORY_EXTERN(LogSIPCharacter, Log, All);
SIP_API DECLARE_LOG_CATEGORY_EXTERN(LogSIPAbilitySystem, Log, All);
