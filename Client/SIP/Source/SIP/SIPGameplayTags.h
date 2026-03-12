// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * SIPGameplayTags 定义了项目中使用的所有 GameplayTag
 * 
 * 什么是 GameplayTag？
 * - UE 的标签系统，用于标识和分类游戏对象
 * - 类似于字符串，但支持层级结构（如 InputTag.Dash）
 * - 高效的匹配和查找
 * 
 * GameplayTag 的优势：
 * - 比 FName 更快
 * - 支持层级匹配（HasTagExact vs HasTag）
 * - 可配置、可扩展
 * - 网络复制友好
 * 
 * 本项目的标签分类：
 * 1. InputTag - 输入相关
 * 2. State - 角色状态
 * 3. System - 系统控制
 * 4. Health - 生命值系统
 * 5. Vitality - 增益/减益效果
 * 6. Cooldown - 技能冷却
 */

#pragma once

#include "NativeGameplayTags.h"

/**
 * Z 说明：
 * SIPGameplayTags 是标签命名空间
 * 使用 UE_DECLARE_GAMEPLAY_TAG_EXTERN 声明标签
 * 使用 UE_DEFINE_GAMEPLAY_TAG_COMMENT 定义标签
 */
namespace SIPGameplayTags
{
	// Z 说明：运行时根据字符串查找标签（一般不用，仅作备用）
	SIP_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// ==================== Input Tags ====================
	// Z 说明：输入相关标签，用于绑定输入事件到 Ability
	// 这些标签与 InputConfig 中的 FSIPInputAction 配合使用
	// 数据流：按键按下 → InputAction → InputTag → 激活 Ability
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Walk);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Dash);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Potion_Heal);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);

	// 删除注释：InputTag_Look_Stick（暂不支持手柄视角，留作后续扩展）

	// ==================== State Tags ====================
	// Z 说明：角色状态标签，用于标记当前状态
	// 使用场景：技能可以检查这些标签来决定是否激活
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Jumping);
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);


	// ==================== System Tags ====================
	// Z 说明：系统控制标签，用于全局控制
	// 用途：通过 GE 可以禁止玩家输入，常用于被控制/眩晕等状态
	// 使用方式：GE 添加此标签 → ASC 禁止所有技能输入
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

	// ==================== Health Tags ====================
	// Z 说明：生命值系统标签
	// 用途：GAS 中属性变化需要通过标签进行追踪和广播，便于 UI 和其他系统响应
	// 使用场景：
	// - Health.Changed: 血量变化时广播，UI 监听更新血条
	// - Damage: 受到伤害时广播，可能触发受击动画
	// - Death: 死亡状态标记，AI 系统可据此停止攻击
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_Changed);      // 血量变化
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_MaxChanged);   // 最大血量变化
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);              // 受到伤害
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);               // 死亡状态
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStarted);        // 开始死亡
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStopped);        // 复活（死亡结束）

	// ==================== Vitality Tags ====================
	// Z 说明：生命力/增益效果标签
	// 用途：GE 通过标签来标识效果类型，便于技能系统分类管理
	// 使用场景：
	// - Vitality.Healing: 治疗效果，可叠加（如装备回血+技能回血）
	// - Vitality.Burning: 燃烧 debuff，每秒扣血
	// - Vitality.SpeedBoost: 加速效果，移动速度属性修改
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Healing);    // 治疗中
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Burning);    // 燃烧debuff
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_SpeedBoost); // 加速buff

	// ==================== Element Tags ====================
	// 元素标签：药水携带的元素类型 + 世界 ZoneActor 的自身元素
	// 反应计算：Zone.ElementTag + Projectile.ElementTag → ReactionTag
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Fire);     // 火元素（火焰草系药水）
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Ice);      // 冰元素（冰霜藤系药水）
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Thunder);  // 雷元素（雷晶矿系药水）
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Wind);     // 风元素（风羽草系药水）
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Plant);    // 植物元素（森林区域/植物类药水）
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Heal);     // 水/治愈元素（月光花系药水，兼水属性）

	// ==================== Biome Tags ====================
	// 生物群落标签：用于 PCG 岛屿生成时区分岛屿类型
	// 由 SIPIslandGeneratorComponent 使用，不参与反应计算
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Biome_Fire);       // 火山岛
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Biome_Ice);        // 冰原岛
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Biome_Forest);     // 森林岛
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Biome_Plains);     // 平原岛

	// ==================== Reaction Tags ====================
	// 反应结果标签：由 SIPElementReactionSubsystem 计算输出
	// ZoneActor 根据此标签决定 PCG 清理/网格替换/VFX 播放方案
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction_Burn);      // 燃烧：Plant+Fire  → 植被移除，道路开启
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction_Melt);      // 熔化：Ice+Fire    → 冰结构坍塌，隐藏区域显现
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction_Freeze);    // 冻结：Heal+Ice    → 水面冰封，形成可行走平台
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction_Electrify); // 感电：Heal+Thunder→ 水域区域敌人全体眩晕
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction_Bloom);     // 绽放：Plant+Wind  → 孢子扩散，资源点新生

	// ==================== Zone State Tags ====================
	// 区域状态标签：ZoneActor 发生反应后持有此 Tag，防止重复触发
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Zone_Burning);       // 燃烧状态
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Zone_Frozen);        // 冻结状态
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Zone_Electrified);   // 感电状态
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Zone_Bloomed);       // 绽放状态

	// ==================== Cooldown Tags ====================
	// Z 说明：冷却时间标签
	// 用途：标识技能的冷却状态
	// 使用场景：加到对应的cooldown GE里，GA会自动检查这个标签来判断技能是否在冷却中
	SIP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash);
};
