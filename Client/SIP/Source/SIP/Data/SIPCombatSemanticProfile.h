// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/SIPCombatSemanticResolver.h"
#include "SIPCombatSemanticProfile.generated.h"

/**
 * 战斗语义调参数据资产。
 *
 * 把原先散在 Resolver 常量、Bridge UPROPERTY、Locomotion 参数里的阈值，
 * 收口到一个可在编辑器中统一调试的数据资产。
 *
 * 当前仅覆盖 Ice Rune Dagger 黄金链所需的阈值。
 * 后续扩展第二把武器时，再考虑做成 per-weapon profile array。
 */
UCLASS(BlueprintType)
class SIP_API USIPCombatSemanticProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ---- Momentum Band ----

	/** 速度低于此值为 Low 段 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|MomentumBand", meta = (ClampMin = "0.0"))
	float MomentumMidThreshold = 140.0f;

	/** 速度高于此值为 High 段 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|MomentumBand", meta = (ClampMin = "0.0"))
	float MomentumHighThreshold = 260.0f;

	// ---- Turn Demand ----

	/** 角度小于此值视为正前方 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|TurnDemand", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ForwardTurnThresholdDegrees = 30.0f;

	/** 角度大于此值视为硬转 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|TurnDemand", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float HardTurnThresholdDegrees = 90.0f;

	// ---- Ice Rune Dagger ----

	/** 触发 SlideEntry 或 DriftSlash 所需的最低地面速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|IceRuneDagger", meta = (ClampMin = "0.0"))
	float IceRuneDaggerHighMomentumThreshold = 260.0f;

	/** DriftTurnSlash 判定需要的最小绝对转角 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|IceRuneDagger", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float IceRuneDaggerDriftTurnMinAngleDegrees = 50.0f;

	/** 尾态 DelayedRestart 需要的最低速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|IceRuneDagger", meta = (ClampMin = "0.0"))
	float IceRuneDaggerDelayedRestartMinSpeed = 180.0f;

	/** 尾态 GlideExit 需要的最低速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|IceRuneDagger", meta = (ClampMin = "0.0"))
	float IceRuneDaggerGlideExitMinSpeed = 120.0f;

	// ---- Tail State ----

	/** 语义尾态最大存活时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|TailState", meta = (ClampMin = "0.1"))
	float SemanticTailStateTTLSeconds = 2.5f;

	/** 攻击蒙太奇后 MM 抑制安全超时（秒），正常由事件清除 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|TailState", meta = (ClampMin = "0.0"))
	float PostAttackMMSuppressionGraceSeconds = 2.0f;

	// ---- PoseSearch Database Tags ----

	/** 默认 locomotion PoseSearch 数据库标签 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|PoseSearch", meta = (Categories = "PoseSearch.Database"))
	FGameplayTag PoseSearchDatabaseTag_Default;

	/** 冰面 locomotion PoseSearch 数据库标签 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|PoseSearch", meta = (Categories = "PoseSearch.Database"))
	FGameplayTag PoseSearchDatabaseTag_IceLocomotion;

	/** 冰面战斗 PoseSearch 数据库标签 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic|PoseSearch", meta = (Categories = "PoseSearch.Database"))
	FGameplayTag PoseSearchDatabaseTag_IceCombat;

	// ---- Convenience Resolvers ----

	ESIPMomentumBand ResolveMomentumBand(float GroundSpeed) const;
	ESIPTurnDemand ResolveTurnDemand(float AbsoluteTurnAngleDegrees) const;
	ESIPBalanceState ResolveBalanceState(bool bOnIceSurface, ESIPMomentumBand MomentumBand, ESIPTurnDemand TurnDemand, bool bIsRecoverPhase) const;

	/**
	 * 根据 SemanticLocomotionMode 返回对应的 PoseSearch 数据库标签。
	 */
	FGameplayTag GetPoseSearchDatabaseTagForMode(ESIPSemanticLocomotionMode Mode) const;
};
