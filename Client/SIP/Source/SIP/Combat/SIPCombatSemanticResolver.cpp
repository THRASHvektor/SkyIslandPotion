// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/SIPCombatSemanticResolver.h"

#include "Character/SIPHeroCharacter.h"
#include "Data/SIPCombatSemanticProfile.h"
#include "SIPGameplayTags.h"

namespace
{
	// 第一版阈值先保留在本地常量里，等形状稳定后再考虑抽成数据。
	constexpr float MomentumMidThreshold = 140.0f;
	constexpr float MomentumHighThreshold = 260.0f;
	constexpr float ForwardTurnThresholdDegrees = 30.0f;
	constexpr float HardTurnThresholdDegrees = 90.0f;

	/**
	 * 这些小型 tag 判断函数存在的目的，
	 * 是让后面的规则表部分更容易阅读。
	 */
	bool IsRecoverPhaseTag(const FGameplayTag& CombatPhaseTag)
	{
		return CombatPhaseTag.MatchesTagExact(SIPGameplayTags::State_Combat_Cast_Recover);
	}

	bool IsPreCastPhaseTag(const FGameplayTag& CombatPhaseTag)
	{
		return CombatPhaseTag.MatchesTagExact(SIPGameplayTags::State_Combat_Cast_PreCast);
	}

	bool IsReleasePhaseTag(const FGameplayTag& CombatPhaseTag)
	{
		return CombatPhaseTag.MatchesTagExact(SIPGameplayTags::State_Combat_Cast_Release);
	}

	/**
	 * 黄金链只适用于我们当前正在验证的第一块窄域：
	 * 地面 Ice Rune Dagger 战斗。
	 */
	bool IsIceRuneDaggerEligibleState(const FSIPCombatFeatureVector& Vector)
	{
		return
			Vector.SurfaceSemantic.MatchesTagExact(SIPGameplayTags::State_Surface_Ice) &&
			Vector.WeaponModuleTag.MatchesTagExact(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) &&
			Vector.SpatialDemand == ESIPSpatialDemand::GroundChain;
	}

	/**
	 * 尾态允许比主动施法阶段活得更久一点，
	 * 这样攻击本体结束后仍然能表达 delayed restart 或 glide exit。
	 */
	bool IsIceRuneDaggerTailBodyState(const FGameplayTag& BodyStateTag)
	{
		return
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_SlipRecovery) ||
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DelayedRestart) ||
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_GlideExit);
	}

	/**
	 * 所有黄金链里可能出现的 BodyState，
	 * 包括主动施法阶段（SlideEntry / DriftSlash / DriftTurn）和尾态。
	 * 攻击结束后任何一个黄金链状态都应当能生成尾态过渡，而不是直接塌回 None。
	 */
	bool IsIceRuneDaggerGoldenPathBodyState(const FGameplayTag& BodyStateTag)
	{
		return
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_SlideEntry) ||
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DriftSlash) ||
			BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DriftTurn) ||
			IsIceRuneDaggerTailBodyState(BodyStateTag);
	}

	/**
	 * 第一版变体选择刻意保持得很小：只有前向、左、右。
	 */
	FName ResolveDirectionalVariant(const FSIPCombatFeatureVector& Vector)
	{
		if (Vector.TurnDemand == ESIPTurnDemand::Forward)
		{
			return SIPCombatSemantic::VariantForward;
		}

		return Vector.SignedTurnAngleDegrees < 0.0f
			? SIPCombatSemantic::VariantRight
			: SIPCombatSemantic::VariantLeft;
	}
}

namespace SIPCombatSemantic
{
	const FName VariantForward(TEXT("Fwd"));
	const FName VariantLeft(TEXT("Left"));
	const FName VariantRight(TEXT("Right"));
	const FName VariantShort(TEXT("Short"));
	const FName VariantLong(TEXT("Long"));

	/**
	 * gameplay 代码与解析器共用的有符号角度辅助函数。
	 */
	float GetSignedTurnAngleDegrees(const FVector& ForwardVector, const FVector& DesiredDirection)
	{
		const FVector SafeForward2D = ForwardVector.GetSafeNormal2D();
		const FVector SafeDesired2D = DesiredDirection.GetSafeNormal2D();
		if (SafeForward2D.IsNearlyZero() || SafeDesired2D.IsNearlyZero())
		{
			return 0.0f;
		}

		const float Dot = FMath::Clamp(FVector::DotProduct(SafeForward2D, SafeDesired2D), -1.0f, 1.0f);
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
		const float CrossZ = FVector::CrossProduct(SafeForward2D, SafeDesired2D).Z;
		return AngleDegrees * (CrossZ < 0.0f ? -1.0f : 1.0f);
	}

	/**
	 * 第一版粗粒度速度分桶。
	 */
	ESIPMomentumBand ResolveMomentumBand(const float GroundSpeed)
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

	/**
	 * 第一版粗粒度转向需求分桶。
	 */
	ESIPTurnDemand ResolveTurnDemand(const float AbsoluteTurnAngleDegrees)
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

	/**
	 * 估计当前身体表达应该更像稳定、倾斜还是打滑。
	 */
	ESIPBalanceState ResolveBalanceState(const bool bOnIceSurface, const ESIPMomentumBand MomentumBand, const ESIPTurnDemand TurnDemand, const bool bIsRecoverPhase)
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

	/**
	 * 从当前主角状态构建共享特征向量。
	 */
	FSIPCombatFeatureVector BuildHeroCombatFeatureVector(const ASIPHeroCharacter* HeroCharacter, const FGameplayTag& WeaponModuleTag, const FGameplayTag& CombatPhaseTag, const float SignedTurnAngleDegrees)
	{
		return BuildHeroCombatFeatureVector(HeroCharacter, WeaponModuleTag, CombatPhaseTag, SignedTurnAngleDegrees, nullptr);
	}

	/**
	 * 使用显式 Profile 数据资产构建特征向量。
	 * Profile 为 null 时回退到本地默认常量。
	 */
	FSIPCombatFeatureVector BuildHeroCombatFeatureVector(const ASIPHeroCharacter* HeroCharacter, const FGameplayTag& WeaponModuleTag, const FGameplayTag& CombatPhaseTag, const float SignedTurnAngleDegrees, const USIPCombatSemanticProfile* Profile)
	{
		FSIPCombatFeatureVector Vector;
		Vector.SurfaceSemantic =
			(HeroCharacter && HeroCharacter->IsOnIceSurface())
				? SIPGameplayTags::State_Surface_Ice
				: FGameplayTag();
		Vector.WeaponModuleTag = WeaponModuleTag;
		Vector.CombatPhaseTag = CombatPhaseTag;
		Vector.GroundSpeed = HeroCharacter ? HeroCharacter->GetVelocity().Size2D() : 0.0f;
		Vector.SignedTurnAngleDegrees = SignedTurnAngleDegrees;
		Vector.MomentumBand = Profile ? Profile->ResolveMomentumBand(Vector.GroundSpeed) : ResolveMomentumBand(Vector.GroundSpeed);
		Vector.TurnDemand = Profile ? Profile->ResolveTurnDemand(FMath::Abs(SignedTurnAngleDegrees)) : ResolveTurnDemand(FMath::Abs(SignedTurnAngleDegrees));
		Vector.TargetRelation =
			(Vector.TurnDemand == ESIPTurnDemand::Forward)
				? ESIPTargetRelation::FrontClose
				: (Vector.TurnDemand == ESIPTurnDemand::SoftTurn ? ESIPTargetRelation::FrontMid : ESIPTargetRelation::SideClose);
		const bool bIsRecoverPhase = IsRecoverPhaseTag(CombatPhaseTag);
		Vector.BalanceState = Profile
			? Profile->ResolveBalanceState(
				Vector.SurfaceSemantic.MatchesTagExact(SIPGameplayTags::State_Surface_Ice),
				Vector.MomentumBand, Vector.TurnDemand, bIsRecoverPhase)
			: ResolveBalanceState(
				Vector.SurfaceSemantic.MatchesTagExact(SIPGameplayTags::State_Surface_Ice),
				Vector.MomentumBand, Vector.TurnDemand, bIsRecoverPhase);
		Vector.SpatialDemand = ESIPSpatialDemand::GroundChain;
		return Vector;
	}

	/**
	 * 给下游系统使用的域判断函数：
	 * 它们只关心这个描述符是不是属于 Ice Rune Dagger 语义家族。
	 */
	bool IsIceRuneDaggerSemanticDescriptor(const FSIPCombatActionDescriptor& Descriptor)
	{
		return
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_SlideEntry) ||
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_DriftSlash) ||
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_DriftTurnSlash) ||
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_SlipRecovery) ||
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_DelayedRestart) ||
			Descriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_GlideExit) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_SlideEntry) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DriftSlash) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DriftTurn) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_SlipRecovery) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_DelayedRestart) ||
			Descriptor.BodyStateTag.MatchesTagExact(SIPGameplayTags::State_Combat_BodyState_GlideExit);
	}

	/**
	 * Ice Rune Dagger 黄金链的第一版显式规则表。
	 *
	 * 这里的判断顺序非常重要：
	 * 1. 先处理攻击正在进行中的主动施法阶段。
 * 2. 恢复后如果有缓冲输入，再处理 delayed restart。
	 * 3. 如果没有 restart 请求，再退到 glide exit 作为尾态收束。
	 */
	FSIPCombatActionDescriptor ResolveIceRuneDaggerGoldenPath(const FSIPCombatFeatureVector& Vector, const FSIPCombatResolutionContext& Context)
	{
		FSIPCombatActionDescriptor Descriptor;
		const bool bEligibleState = IsIceRuneDaggerEligibleState(Vector);
		const bool bGoldenPathActive =
			bEligibleState &&
			(Vector.MomentumBand == ESIPMomentumBand::High || Context.bLockGoldenPath);
		const bool bPostRecoveryTailActive =
			bEligibleState &&
			IsIceRuneDaggerGoldenPathBodyState(Context.PreviousBodyStateTag) &&
			Vector.GroundSpeed >= Context.GlideExitMinSpeed;
		const bool bBufferedRestartActive =
			Context.bHasBufferedFollowUp &&
			bEligibleState &&
			Vector.GroundSpeed >= Context.DelayedRestartMinSpeed;
		// Graceful chain exit: buffered follow-up from a semantic tail state
		// but speed too low for restart — produce GlideExit instead of None
		// to avoid jarring snap back to basic combo.
		const bool bBufferedGracefulExitActive =
			Context.bHasBufferedFollowUp &&
			bEligibleState &&
			IsIceRuneDaggerGoldenPathBodyState(Context.PreviousBodyStateTag) &&
			!bBufferedRestartActive;

		if (!bGoldenPathActive && !bPostRecoveryTailActive && !bBufferedRestartActive && !bBufferedGracefulExitActive)
		{
			return Descriptor;
		}

		Descriptor.bGoldenPathActive = true;

		if (IsPreCastPhaseTag(Vector.CombatPhaseTag) && bGoldenPathActive)
		{
			Descriptor.ActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_SlideEntry;
			Descriptor.BodyStateTag = SIPGameplayTags::State_Combat_BodyState_SlideEntry;
			Descriptor.DesiredVariant = ResolveDirectionalVariant(Vector);
			Descriptor.bUseMomentumWarp = true;
			return Descriptor;
		}

		if (IsReleasePhaseTag(Vector.CombatPhaseTag) && bGoldenPathActive)
		{
			const bool bForwardStrike = Vector.TurnDemand == ESIPTurnDemand::Forward;
			Descriptor.ActionFamilyTag =
				bForwardStrike
					? SIPGameplayTags::State_Combat_ActionFamily_DriftSlash
					: SIPGameplayTags::State_Combat_ActionFamily_DriftTurnSlash;
			Descriptor.BodyStateTag =
				bForwardStrike
					? SIPGameplayTags::State_Combat_BodyState_DriftSlash
					: SIPGameplayTags::State_Combat_BodyState_DriftTurn;
			Descriptor.DesiredVariant = ResolveDirectionalVariant(Vector);
			Descriptor.bUseMomentumWarp = true;
			Descriptor.RecoveryBias =
				(Vector.TurnDemand == ESIPTurnDemand::HardTurn)
					? ESIPRecoveryBias::SevereSlip
					: ESIPRecoveryBias::Delayed;
			Descriptor.ChainWindowPolicy =
				(Vector.TurnDemand == ESIPTurnDemand::Forward)
					? ESIPChainWindowPolicy::Normal
					: ESIPChainWindowPolicy::Late;
			return Descriptor;
		}

		if (IsRecoverPhaseTag(Vector.CombatPhaseTag) && bGoldenPathActive)
		{
			Descriptor.ActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_SlipRecovery;
			Descriptor.BodyStateTag = SIPGameplayTags::State_Combat_BodyState_SlipRecovery;
			Descriptor.DesiredVariant =
				(Vector.BalanceState == ESIPBalanceState::Slipping)
					? VariantLong
					: VariantShort;
			Descriptor.bUseMomentumWarp = false;
			Descriptor.RecoveryBias =
				(Vector.BalanceState == ESIPBalanceState::Slipping)
					? ESIPRecoveryBias::SevereSlip
					: ESIPRecoveryBias::Delayed;
			Descriptor.ChainWindowPolicy = ESIPChainWindowPolicy::Late;
			return Descriptor;
		}

		if (bBufferedRestartActive)
		{
			Descriptor.ActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DelayedRestart;
			Descriptor.BodyStateTag = SIPGameplayTags::State_Combat_BodyState_DelayedRestart;
			Descriptor.DesiredVariant = ResolveDirectionalVariant(Vector);
			Descriptor.bUseMomentumWarp = true;
			Descriptor.RecoveryBias = ESIPRecoveryBias::Delayed;
			Descriptor.ChainWindowPolicy = ESIPChainWindowPolicy::Early;
			return Descriptor;
		}

		if (bPostRecoveryTailActive)
		{
			Descriptor.ActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_GlideExit;
			Descriptor.BodyStateTag = SIPGameplayTags::State_Combat_BodyState_GlideExit;
			Descriptor.DesiredVariant = ResolveDirectionalVariant(Vector);
			Descriptor.bUseMomentumWarp = false;
			Descriptor.RecoveryBias = ESIPRecoveryBias::Delayed;
			Descriptor.ChainWindowPolicy = ESIPChainWindowPolicy::Late;
			return Descriptor;
		}

		if (bBufferedGracefulExitActive)
		{
			Descriptor.ActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_GlideExit;
			Descriptor.BodyStateTag = SIPGameplayTags::State_Combat_BodyState_GlideExit;
			Descriptor.DesiredVariant = ResolveDirectionalVariant(Vector);
			Descriptor.bUseMomentumWarp = false;
			Descriptor.RecoveryBias = ESIPRecoveryBias::Delayed;
			Descriptor.ChainWindowPolicy = ESIPChainWindowPolicy::Late;
			return Descriptor;
		}

		Descriptor.bGoldenPathActive = false;
		return Descriptor;
	}
}
