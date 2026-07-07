// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/SIPCombatSemanticProfile.h"

#include "SIPGameplayTags.h"

ESIPMomentumBand USIPCombatSemanticProfile::ResolveMomentumBand(const float GroundSpeed) const
{
	if (GroundSpeed < MomentumMidThreshold)
	{
		return ESIPMomentumBand::Low;
	}

	if (GroundSpeed < MomentumHighThreshold)
	{
		return ESIPMomentumBand::Mid;
	}

	return ESIPMomentumBand::High;
}

ESIPTurnDemand USIPCombatSemanticProfile::ResolveTurnDemand(const float AbsoluteTurnAngleDegrees) const
{
	if (AbsoluteTurnAngleDegrees < ForwardTurnThresholdDegrees)
	{
		return ESIPTurnDemand::Forward;
	}

	if (AbsoluteTurnAngleDegrees <= HardTurnThresholdDegrees)
	{
		return ESIPTurnDemand::SoftTurn;
	}

	return ESIPTurnDemand::HardTurn;
}

ESIPBalanceState USIPCombatSemanticProfile::ResolveBalanceState(const bool bOnIceSurface, const ESIPMomentumBand MomentumBand, const ESIPTurnDemand TurnDemand, const bool bIsRecoverPhase) const
{
	if (!bOnIceSurface)
	{
		return ESIPBalanceState::Stable;
	}

	if (bIsRecoverPhase && MomentumBand == ESIPMomentumBand::High)
	{
		return ESIPBalanceState::Slipping;
	}

	if (MomentumBand == ESIPMomentumBand::High)
	{
		return TurnDemand == ESIPTurnDemand::HardTurn
			? ESIPBalanceState::Slipping
			: ESIPBalanceState::Leaning;
	}

	return ESIPBalanceState::Stable;
}

FGameplayTag USIPCombatSemanticProfile::GetPoseSearchDatabaseTagForMode(const ESIPSemanticLocomotionMode Mode) const
{
	switch (Mode)
	{
	case ESIPSemanticLocomotionMode::IceLocomotion:
		// 恢复 PSD_IceLocomotion 独立路由。该 PSD 包含 Dense idles/starts/loops
		// + 8 个 Mage 重定向冰面特殊片段（Fast Start/Stop/Turn/Combat-to-Run 等）。
		// Flow 07 已移除 stops/pivots/turns 以避免与冰面物理冲突，
		// 剩余内容正好提供 momentum-preserving 的冰面移动感。
		return PoseSearchDatabaseTag_IceLocomotion.IsValid()
			? PoseSearchDatabaseTag_IceLocomotion
			: SIPGameplayTags::PoseSearch_Database_IceLocomotion;

	case ESIPSemanticLocomotionMode::IceCombat:
		// PSD_IceCombat 尚未创建，但 PSD_IceLocomotion 已含 Dense
		// idles/starts/loops + 8 Mage 冰面重定向片段。攻击后语义
		// 尾巴路由到 IceLocomotion 避免 Default PSD 通用 walk/run
		// 动画产生"原地走路"伪影。
		// TODO: 创建专用 PSD_IceCombat，加 Combat→Idle 过渡片段。
		return PoseSearchDatabaseTag_IceLocomotion.IsValid()
			? PoseSearchDatabaseTag_IceLocomotion
			: SIPGameplayTags::PoseSearch_Database_IceLocomotion;

	case ESIPSemanticLocomotionMode::SemanticCombatOverride:
		return FGameplayTag();

	case ESIPSemanticLocomotionMode::Default:
	default:
		return PoseSearchDatabaseTag_Default.IsValid()
			? PoseSearchDatabaseTag_Default
			: SIPGameplayTags::PoseSearch_Database_Default;
	}
}
