#pragma once
/**
 * USIPAbilitySet 是项目中的技能集数据资产
 * 继承自 UPrimaryDataAsset，用于存储一组可授予给角色的技能配置
 * 
 * 技能集的作用：
 * 1. 将多个相关的技能打包成一个配置单元
 * 2. 可以通过 Blueprint 配置，无需硬编码
 * 3. 支持授予 Ability、AttributeSet、GameplayEffect
 * 
 * 使用方式：
 * 1. 在 Blueprint 中创建 USIPAbilitySet 数据资产
 * 2. 配置 GrantedGameplayAbilities、GrantedAttributeSets、GrantedGameplayEffects
 * 3. 在角色的 AbilitySets 属性中引用此数据资产
 * 4. 角色初始化时自动授予所有配置的内容
 */

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "SIPAbilitySet.generated.h"


class UGameplayAbility;
class UAttributeSet;
class UAbilitySystemComponent;
struct FGameplayAbilitySpecHandle;

/**
 * FSIPAbilitySet_GameplayAbility 定义单个技能的配置
 * 用于将一个 GameplayAbility 绑定到输入标签
 * 
 * 数据流：
 * InputTag → Ability → 激活技能
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:

	/**
	 * 必须是 UGameplayAbility 或其子类的蓝图类/C++类
	 * 运行时会被实例化为 AbilitySpec
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> Ability = nullptr;

	/**
	 * 当玩家按下对应输入时，ASC 会查找匹配的 Ability 并激活
	 * 
	 * 匹配规则：
	 * - Ability 的 DynamicAbilityTags 必须包含此 InputTag
	 * - 这样可以实现"一个输入标签对应一个技能"
	 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


/**
 * FSIPAbilitySet_GameplayEffect 定义单个 GameplayEffect 的配置
 * 用于授予被动效果或初始属性修饰
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	/**
	 * 常用场景：
	 * - 初始属性设置（如设置最大生命值为100）
	 * - 被动 Buff（如永久加速）
	 * - 持续伤害/治疗效果
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	/**
	 * 某些 GE 支持多等级，等级越高效果越强
	 */
	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};


/**
 * FSIPAbilitySet_AttributeSet 定义单个 AttributeSet 的配置
 * 用于授予角色的属性集
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:

	/**
	 * 每个角色至少需要一个 AttributeSet 来存储属性
	 * 常用：USIPHealthSet（生命值）、自定义属性集
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet = nullptr;
};


/**
 * FSIPAbilitySet_GrantedHandles 用于存储已授权内容的句柄
 * 
 * 为什么需要这个结构？
 * - 当需要移除技能/效果时，需要知道具体的句柄
 * - 类似于"引用计数"，记录授予了多少东西
 * - 移除时按句柄精确移除，不会影响其他技能
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:

	/**
	 * 在授予技能时调用，记录授予的技能
	 */
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);

	/**
	 * 在授予效果时调用，记录授予的效果
	 */
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);

	/**
	 * 在授予属性集时调用，记录授予的属性集
	 */
	void AddAttributeSet(UAttributeSet* AttributeSet);

	/**
	 * 角色死亡/重生时可能需要调用此函数
	 * 会清除所有 Ability、移除 AttributeSet
	 */
	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

protected:

	/**
	 * 用于后续清除技能
	 */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	/**
	 * 用于后续移除效果
	 */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
	
	/**
	 * 用于后续移除属性集
	 */
	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};


/**
 * USIPAbilitySet 是技能集的容器类
 * 包含可授予给角色的 Ability、AttributeSet、GameplayEffect 配置
 * 
 * 设计理念：
 * - 类似"技能配置文件"，在 Blueprint 中配置角色拥有的技能
 * - 角色初始化时读取此配置，授予所有内容
 * - 支持多个 AbilitySet，实现"技能页"功能
 */
UCLASS(BlueprintType, Const)
class USIPAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	USIPAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 这是核心函数，在角色初始化时调用
	 * 
	 * @param ASC - 目标 AbilitySystemComponent
	 * @param OutGrantedHandles - 输出参数，返回已授予内容的句柄
	 * @param SourceObject - 源对象（如角色 actor）
	 * 
	 * 授予流程：
	 * 1. 遍历 GrantedGameplayAbilities，创建并授予技能
	 * 2. 遍历 GrantedAttributeSets，创建并添加属性集
	 * 3. 遍历 GrantedGameplayEffects，应用效果
	 */
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSIPAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:

	/**
	 * 每个元素包含 Ability 类和 InputTag
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FSIPAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	/**
	 * 通常每个角色需要至少一个属性集
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Attributes", meta=(TitleProperty=AttributeSet))
	TArray<FSIPAbilitySet_AttributeSet> GrantedAttributeSets;

	/**
	 * 用于初始属性设置或被动效果
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FSIPAbilitySet_GameplayEffect> GrantedGameplayEffects;
};
