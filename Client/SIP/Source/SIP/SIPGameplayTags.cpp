// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * SIPGameplayTags.cpp 是 GameplayTag 的定义文件
 * 使用 UE_DEFINE_GAMEPLAY_TAG_COMMENT 宏定义所有标签
 * 
 * 标签定义格式：
 * UE_DEFINE_GAMEPLAY_TAG_COMMENT(标签名, "标签字符串", "描述")
 * 
 * 标签字符串规则：
 * - 使用点号分层级（如 InputTag.Dash）
 * - 建议按功能模块分组
 */

#include "SIPGameplayTags.h"

#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"
#include "SIPLogCategory.h"

namespace SIPGameplayTags
{
	// ==================== Input Tags ====================
	// Z 说明：输入相关标签，用于绑定输入事件到 Ability
	// 这些标签与 InputConfig 配合，实现输入到技能的映射
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint", "Sprint input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dash, "InputTag.Dash", "Dash input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "Attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Potion_Heal, "InputTag.Potion.Heal", "Healing potion input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Walk, "InputTag.Walk", "Walk input.");

	// InputTag_Look_Stick 的定义（原因：暂不支持手柄视角控制，后续需要时可重新添加）

	// ==================== State Tags ====================
	// Z 说明：角色状态标签，用于标记当前状态
	// 使用场景：技能可以检查这些标签来决定是否激活或改变行为
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Jumping, "State.Movement.Jumping", "Jumping state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead.");

	// ==================== System Tags ====================
	// Z 说明：系统控制标签，用于全局控制
	// 用途：通过 GE 可以禁止玩家输入，常用于被控制/眩晕等状态
	UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

	// ==================== Health Tags ====================
	// Z 说明：生命值系统标签
	// 用途：GAS 中属性变化需要通过标签进行追踪和广播，便于 UI 和其他系统响应
	// 使用场景：
	// - Health.Changed: 血量变化时广播，UI 监听更新血条
	// - Damage: 受到伤害时广播，可能触发受击动画
	// - Death: 死亡状态标记，AI 系统可据此停止攻击
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_Changed, "Health.Changed", "Health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_MaxChanged, "Health.MaxChanged", "Max health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Damage taken.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Death", "Character death.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStarted, "Death.Started", "Death has started.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStopped, "Death.Stopped", "Death has stopped (revive).");

	// ==================== Vitality Tags ====================
	// Z 说明：生命力/增益效果标签
	// 用途：GE 通过标签来标识效果类型，便于技能系统分类管理
	// 使用场景：
	// - Vitality.Healing: 治疗效果，可叠加
	// - Vitality.Burning: 燃烧 debuff，每秒扣血
	// - Vitality.SpeedBoost: 加速效果
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Healing, "Vitality.Healing", "Healing over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Burning, "Vitality.Burning", "Burning damage over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_SpeedBoost, "Vitality.SpeedBoost", "Movement speed boost.");

	// ==================== Tag Lookup Function ====================
	// Z 说明：运行时根据字符串查找标签
	// 一般不使用，仅作备用（通过字符串动态获取标签）
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
