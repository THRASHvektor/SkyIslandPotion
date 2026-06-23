#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Ability/SIPGameplayAbility_Attack.h"
#include "SIPAttackComboDataAsset.generated.h"

/**
 * 攻击能力使用的连招数据资产容器。
 *
 * 当前这个类除了存数据，还承担了一层防御式自动补全逻辑。
 * 原因是项目里已经存在 Rune Dagger 连招资产，
 * 它们的数组可能看起来“有条目”，
 * 但结构体内部实际上仍然是语义上的空壳。
 */
UCLASS(BlueprintType)
class SIP_API USIPAttackComboDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * 新建资产时如果名称匹配 RuneDagger，也自动填充初始连招。
	 */
	virtual void PostInitProperties() override;

	/**
	 * 资源加载后修复已知为空壳的 Rune Dagger 连招资产，
	 * 让运行时和编辑器里看到的都是可用的默认连招。
	 */
	virtual void PostLoad() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (Categories = "State.Combat.WeaponModule"))
	FGameplayTag DefaultWeaponModuleTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.05"))
	float ComboResetWindowSeconds = 2.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float BufferedComboInputWindowSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<FSIPAttackComboEntry> ComboEntries;

	/**
	 * 攻击能力和资源自动修复逻辑共用的 Rune Dagger 内建连招列表。
	 */
	static const TArray<FSIPAttackComboEntry>& BuildRuneDaggerAttackComboEntriesV2();

	/**
	 * 区分“真正写过内容的连招数组”和“只有空占位结构体的数组”。
	 */
	static bool HasMeaningfulComboEntries(const TArray<FSIPAttackComboEntry>& Entries);

private:
	/**
	 * 当已知 Rune Dagger 资产以“结构上存在、内容上为空”的状态加载时，
	 * 自动补入默认内容，并标脏包体，方便后续直接保存。
	 */
	void AutoPopulateDefaultsIfNeeded();
};
