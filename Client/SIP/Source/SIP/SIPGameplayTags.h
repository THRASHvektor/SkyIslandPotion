// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace SIPGameplayTags
{
	SIP_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// ==================== Input Tags ====================
	// 输入相关Tag，用于绑定输入事件到Ability
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Walk);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dash);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash);

	// 删除注释：InputTag_Look_Stick（暂不支持手柄视角，留作后续扩展）

	// ==================== State Tags ====================
	// 状态相关Tag，用于标记角色当前状态
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Jumping);


	// ==================== System Tags ====================
	// 系统控制Tag，用于全局控制
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

	// ==================== Health Tags ====================
	// 新增：生命值系统Tag
	// GAS中属性变化需要通过Tag进行追踪和广播，便于UI和其他系统响应
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_Changed);      // 血量变化
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_MaxChanged);   // 最大血量变化
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);              // 受到伤害
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);               // 死亡状态
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStarted);        // 开始死亡
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStopped);        // 复活（死亡结束）

	// ==================== Vitality Tags ====================
	// 新增：生命力/增益效果Tag（用于Buff/Debuff系统）
	// GE通过Tag来标识效果类型，便于技能系统分类管理
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Healing);    // 治疗中
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Burning);    // 燃烧debuff
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_SpeedBoost); // 加速buff
};
