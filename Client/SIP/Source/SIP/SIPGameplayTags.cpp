// Copyright Epic Games, Inc. All Rights Reserved.
/**
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
	// ==================== 输入标签 ====================
	// 这些标签与 InputConfig 配合，实现输入到技能的映射
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint", "Sprint input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Aim, "InputTag.Aim", "Aim input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Strafe, "InputTag.Strafe", "Strafe input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "Crouch input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Dash, "InputTag.Dash", "Dash input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "Attack input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Potion_Heal, "InputTag.Potion.Heal", "Healing potion input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Walk, "InputTag.Walk", "Walk input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "Interact input.");

	// InputTag_Look_Stick 的定义（原因：暂不支持手柄视角控制，后续需要时可重新添加）

	// ==================== 状态标签 ====================
	// 使用场景：技能可以检查这些标签来决定是否激活或改变行为
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat, "State.Combat", "Character is in combat presentation state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Attacking, "State.Combat.Attacking", "Character is playing an attack action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Throwing, "State.Combat.Throwing", "Character is playing a throw action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Attack_HitWindow, "State.Combat.Attack.HitWindow", "Attack hit window is currently open.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Cast_PreCast, "State.Combat.Cast.PreCast", "Combat action is in the pre-cast or wind-up phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Cast_Release, "State.Combat.Cast.Release", "Combat action is in the release or hit phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_Cast_Recover, "State.Combat.Cast.Recover", "Combat action is in the recovery phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_WeaponModule_Unarmed, "State.Combat.WeaponModule.Unarmed", "Current combat expression uses the unarmed module profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_WeaponModule_FlaskRig, "State.Combat.WeaponModule.FlaskRig", "Current combat expression uses the flask rig module profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_WeaponModule_RuneDagger, "State.Combat.WeaponModule.RuneDagger", "Current combat expression uses the rune dagger module profile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_SlideEntry, "State.Combat.ActionFamily.SlideEntry", "Combat action resolves to the ice rune dagger slide entry family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_DriftSlash, "State.Combat.ActionFamily.DriftSlash", "Combat action resolves to the ice rune dagger drift slash family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_DriftTurnSlash, "State.Combat.ActionFamily.DriftTurnSlash", "Combat action resolves to the ice rune dagger drift turn slash family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_SlipRecovery, "State.Combat.ActionFamily.SlipRecovery", "Combat action resolves to the ice rune dagger slip recovery family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_DelayedRestart, "State.Combat.ActionFamily.DelayedRestart", "Combat action resolves to the ice rune dagger delayed restart family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_ActionFamily_GlideExit, "State.Combat.ActionFamily.GlideExit", "Combat action resolves to the ice rune dagger glide exit family.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_SlideEntry, "State.Combat.BodyState.SlideEntry", "Current combat action enters through an ice-driven sliding attack state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_SlipRecovery, "State.Combat.BodyState.SlipRecovery", "Current combat action is recovering from an ice-driven slipping motion.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_DriftSlash, "State.Combat.BodyState.DriftSlash", "Current combat action expresses a drifting slash body state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_DriftTurn, "State.Combat.BodyState.DriftTurn", "Current combat action expresses a drift-assisted turning strike.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_DelayedRestart, "State.Combat.BodyState.DelayedRestart", "Current combat action is restarting from a delayed recovery state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Combat_BodyState_GlideExit, "State.Combat.BodyState.GlideExit", "Current combat action is exiting a glide-driven combat chain.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Jumping, "State.Movement.Jumping", "Jumping state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Aiming, "State.Movement.Aiming", "Character is currently aiming.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Strafing, "State.Movement.Strafing", "Character is currently strafing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Crouching, "State.Movement.Crouching", "Character is currently crouching.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Traversing, "State.Movement.Traversing", "Character is performing a traversal action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Surface_Ice, "State.Surface.Ice", "Character is currently moving on an ice-like surface.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Character is dead.");

	// ==================== 动画事件标签 ====================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Animation_Attack_Request, "Event.Animation.Attack.Request", "Animation request for an attack action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Animation_Attack_HitWindow_Start, "Event.Animation.Attack.HitWindow.Start", "Animation opened the attack hit window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Animation_Attack_HitWindow_End, "Event.Animation.Attack.HitWindow.End", "Animation closed the attack hit window.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Animation_Throw_Request, "Event.Animation.Throw.Request", "Animation request for a throw action.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Animation_Throw_Release, "Event.Animation.Throw.Release", "Animation reached the projectile release frame.");

	// ==================== 系统标签 ====================
	// 用途：通过 GE 可以禁止玩家输入，常用于被控制/眩晕等状态
	UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

	// ==================== 生命值标签 ====================
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

	// ==================== 生命力标签 ====================
	// 用途：GE 通过标签来标识效果类型，便于技能系统分类管理
	// 使用场景：
	// - Vitality.Healing: 治疗效果，可叠加
	// - Vitality.Burning: 燃烧 debuff，每秒扣血
	// - Vitality.SpeedBoost: 加速效果
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Healing, "Vitality.Healing", "Healing over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Burning, "Vitality.Burning", "Burning damage over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_SpeedBoost, "Vitality.SpeedBoost", "Movement speed boost.");

	// ==================== 元素标签 ====================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Fire,    "Element.Fire",    "Fire element - carried by fire potions and volcanic zones.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Ice,     "Element.Ice",     "Ice element - carried by ice potions and frozen zones.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Thunder, "Element.Thunder", "Thunder element - carried by thunder potions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Wind,    "Element.Wind",    "Wind element - carried by wind potions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Plant,   "Element.Plant",   "Plant element - forest zones and nature potions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Heal,    "Element.Heal",    "Heal/Water element - healing potions, acts as water in reactions.");

	// ==================== 群落标签 ====================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Biome_Fire,   "Biome.Fire",   "Volcanic island biome - used by PCG island generator.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Biome_Ice,    "Biome.Ice",    "Frozen island biome - used by PCG island generator.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Biome_Forest, "Biome.Forest", "Forest island biome - used by PCG island generator.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Biome_Plains, "Biome.Plains", "Plains island biome - used by PCG island generator.");

	// ==================== 反应标签 ====================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Burn,      "Reaction.Burn",      "Plant+Fire:  vegetation removed, path revealed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Melt,      "Reaction.Melt",      "Ice+Fire:    ice structures melt, hidden area exposed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Freeze,    "Reaction.Freeze",    "Heal+Ice:    water surface freezes into walkable platform.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Electrify, "Reaction.Electrify", "Heal+Thunder: wet area electrified, enemies stunned.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reaction_Bloom,     "Reaction.Bloom",     "Plant+Wind:  spores spread, new resource nodes appear.");

	// ==================== 区域状态标签 ====================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Zone_Burning,     "Zone.Burning",     "Zone has been burned - vegetation cleared.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Zone_Frozen,      "Zone.Frozen",      "Zone has been frozen - ice platform active.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Zone_Electrified, "Zone.Electrified", "Zone is electrified - enemies in area are stunned.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Zone_Bloomed,     "Zone.Bloomed",     "Zone has bloomed - new resources spawned.");

	// ==================== 冷却标签 ====================
	// 用途：标识技能的冷却状态
	// 使用场景：加到对应的cooldown GE里，GA会自动检查这个标签来判断技能是否在冷却中
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash, "Cooldown.Dash", "Dash cooldown.");

	// ==================== 标签查找函数 ====================
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
