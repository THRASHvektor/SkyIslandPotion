// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPGameplayTags.h"

#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"
#include "SIPLogCategory.h"

namespace SIPGameplayTags
{
	// ==================== Input Tags ====================
	// 输入相关Tag，用于绑定输入事件到Ability
	// 修改说明：添加分组注释，统一整理格式
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint", "Sprint input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dash, "InputTag.Dash", "Dash input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Walk, "InputTag.Walk", "Walk input.");

	// InputTag_Look_Stick 的定义（原因：暂不支持手柄视角控制，后续需要时可重新添加）

	// ==================== State Tags ====================
	// 状态相关Tag，用于标记角色当前状态
	// 修改说明：添加分组注释
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Jumping, "State.Movement.Jumping", "Jumping state.");

	// ==================== System Tags ====================
	// 系统控制Tag，用于全局控制
	// 用途：通过GE可以禁止玩家输入，常用于被控制/眩晕等状态
	UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

	// ==================== Health Tags ====================
	// 新增：生命值系统Tag
	// 理由：GAS中属性变化需要通过Tag进行追踪和广播，便于UI和其他系统响应
	// 使用场景：
	//   - Health.Changed: 血量变化时广播，UI监听更新血条
	//   - Damage: 受到伤害时广播，可能触发受击动画
	//   - Death: 死亡状态标记，AI系统可据此停止攻击
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_Changed, "Health.Changed", "Health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_MaxChanged, "Health.MaxChanged", "Max health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Damage taken.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Death", "Character death.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStarted, "Death.Started", "Death has started.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStopped, "Death.Stopped", "Death has stopped (revive).");

	// ==================== Vitality Tags ====================
	// 新增：生命力/增益效果Tag（用于Buff/Debuff系统）
	// 理由：GE通过Tag来标识效果类型，便于技能系统分类管理
	// 使用场景：
	//   - Vitality.Healing: 治疗效果，可叠加（如装备回血+技能回血）
	//   - Vitality.Burning: 燃烧debuff，每秒扣血
	//   - Vitality.SpeedBoost: 加速效果，移动速度属性修改
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Healing, "Vitality.Healing", "Healing over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Burning, "Vitality.Burning", "Burning damage over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_SpeedBoost, "Vitality.SpeedBoost", "Movement speed boost.");

	// ==================== Tag Lookup Function ====================
	// 保留原有的Tag查找功能，用于运行时动态查找Tag
	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(LogSIP, Display, TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."), *TagString, *TestTag.ToString());
					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}
}
