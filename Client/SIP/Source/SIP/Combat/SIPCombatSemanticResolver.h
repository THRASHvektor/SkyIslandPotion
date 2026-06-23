// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SIPCombatSemanticResolver.generated.h"

class ASIPHeroCharacter;
class USIPCombatSemanticProfile;

/**
 * 语义系统建议的 locomotion 模式，用于 ABP 选择 PoseSearchDatabase。
 */
UENUM(BlueprintType)
enum class ESIPSemanticLocomotionMode : uint8
{
	Default,
	IceLocomotion,
	IceCombat,
	SemanticCombatOverride
};

/**
 * 为了让第一版解析器保持可读性而使用的粗粒度动量分段。
 */
UENUM(BlueprintType)
enum class ESIPMomentumBand : uint8
{
	Low,
	Mid,
	High
};

/**
 * 当前攻击需要修正朝向的强度。
 */
UENUM(BlueprintType)
enum class ESIPTurnDemand : uint8
{
	Forward,
	SoftTurn,
	HardTurn
};

/**
 * 第一版语义向量里预留的小型目标关系字段。
 */
UENUM(BlueprintType)
enum class ESIPTargetRelation : uint8
{
	FrontClose,
	FrontMid,
	SideClose
};

/**
 * 当前身体表达使用的简化稳定性模型。
 */
UENUM(BlueprintType)
enum class ESIPBalanceState : uint8
{
	Stable,
	Leaning,
	Slipping
};

/**
 * 预留给未来空间需求扩展的字段，当前只覆盖地面连段。
 */
UENUM(BlueprintType)
enum class ESIPSpatialDemand : uint8
{
	GroundChain
};

/**
 * 提供给后续表现层消费的恢复强度提示。
 */
UENUM(BlueprintType)
enum class ESIPRecoveryBias : uint8
{
	Fast,
	Delayed,
	SevereSlip
};

/**
 * 下一段连招窗口应当有多宽松的提示信息。
 */
UENUM(BlueprintType)
enum class ESIPChainWindowPolicy : uint8
{
	Early,
	Normal,
	Late
};

/**
 * 第一版战斗语义解析器使用的小型运行时特征向量。
 *
 * 这里刻意保持得很窄。
 * 目标不是现在就覆盖整个战斗空间，
 * 而是先为一条黄金链提供稳定、共享、可检查的输入。
 */
USTRUCT(BlueprintType)
struct SIP_API FSIPCombatFeatureVector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Surface"))
	FGameplayTag SurfaceSemantic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Combat.WeaponModule"))
	FGameplayTag WeaponModuleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Combat.Cast"))
	FGameplayTag CombatPhaseTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float GroundSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float SignedTurnAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPMomentumBand MomentumBand = ESIPMomentumBand::Low;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPTurnDemand TurnDemand = ESIPTurnDemand::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPTargetRelation TargetRelation = ESIPTargetRelation::FrontMid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPBalanceState BalanceState = ESIPBalanceState::Stable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPSpatialDemand SpatialDemand = ESIPSpatialDemand::GroundChain;
};

/**
 * 不适合直接放进原始特征向量里的额外运行时上下文。
 *
 * 这里主要补充：
 * 1. 上一帧的语义尾态。
 * 2. 缓冲连招输入意图。
 * 3. 一次攻击进行中时的临时锁定行为。
 */
USTRUCT(BlueprintType)
struct SIP_API FSIPCombatResolutionContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Combat.BodyState"))
	FGameplayTag PreviousBodyStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bHasBufferedFollowUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bLockGoldenPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float DelayedRestartMinSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float GlideExitMinSpeed = 120.0f;
};

/**
 * 解析器返回的共享语义答案。
 *
 * 这个描述符的目标是让：
 * - gameplay，
 * - animation bridge，
 * - 以及后续的 AnimBP / Chooser
 *
 * 都能消费同一份结论，
 * 而不是各自再从原始移动值里重复推导一遍。
 */
USTRUCT(BlueprintType)
struct SIP_API FSIPCombatActionDescriptor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Combat.ActionFamily"))
	FGameplayTag ActionFamilyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "State.Combat.BodyState"))
	FGameplayTag BodyStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName DesiredVariant = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bUseMomentumWarp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPRecoveryBias RecoveryBias = ESIPRecoveryBias::Fast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ESIPChainWindowPolicy ChainWindowPolicy = ESIPChainWindowPolicy::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bGoldenPathActive = false;

	bool HasResolvedAction() const
	{
		return bGoldenPathActive && (ActionFamilyTag.IsValid() || BodyStateTag.IsValid());
	}
};

namespace SIPCombatSemantic
{
	// 资源侧连招条目和解析器输出共用的变体名称。
	extern SIP_API const FName VariantForward;
	extern SIP_API const FName VariantLeft;
	extern SIP_API const FName VariantRight;
	extern SIP_API const FName VariantShort;
	extern SIP_API const FName VariantLong;

	/**
	 * 计算面朝方向到目标移动/攻击方向的二维有符号夹角。
	 */
	SIP_API float GetSignedTurnAngleDegrees(const FVector& ForwardVector, const FVector& DesiredDirection);

	/**
	 * 把连续速度压缩成第一版可读的动量分段。
	 */
	SIP_API ESIPMomentumBand ResolveMomentumBand(float GroundSpeed);

	/**
	 * 把连续转角压缩成第一版可读的转向需求分段。
	 */
	SIP_API ESIPTurnDemand ResolveTurnDemand(float AbsoluteTurnAngleDegrees);

	/**
	 * 估计当前身体表达应当更像稳定、倾斜还是打滑。
	 */
	SIP_API ESIPBalanceState ResolveBalanceState(bool bOnIceSurface, ESIPMomentumBand MomentumBand, ESIPTurnDemand TurnDemand, bool bIsRecoverPhase);

	/**
	 * 直接从主角运行时状态构造第一版特征向量。
	 */
	SIP_API FSIPCombatFeatureVector BuildHeroCombatFeatureVector(const ASIPHeroCharacter* HeroCharacter, const FGameplayTag& WeaponModuleTag, const FGameplayTag& CombatPhaseTag, float SignedTurnAngleDegrees);

	/**
	 * 使用 CombatSemanticProfile 数据资产中的阈值构造特征向量。
	 * 当 Profile 为 null 时回退到内部默认常量。
	 */
	SIP_API FSIPCombatFeatureVector BuildHeroCombatFeatureVector(const ASIPHeroCharacter* HeroCharacter, const FGameplayTag& WeaponModuleTag, const FGameplayTag& CombatPhaseTag, float SignedTurnAngleDegrees, const USIPCombatSemanticProfile* Profile);

	/**
	 * 供调用方判断某个描述符是否属于 Ice Rune Dagger 语义域。
	 */
	SIP_API bool IsIceRuneDaggerSemanticDescriptor(const FSIPCombatActionDescriptor& Descriptor);

	/**
	 * Ice Rune Dagger 黄金链的第一版显式语义解析函数。
	 */
	SIP_API FSIPCombatActionDescriptor ResolveIceRuneDaggerGoldenPath(const FSIPCombatFeatureVector& Vector, const FSIPCombatResolutionContext& Context = FSIPCombatResolutionContext());
}
