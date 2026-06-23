#include "Data/SIPAttackComboDataAsset.h"

#include "SIPGameplayTags.h"

namespace
{
	static const FName LegacyFullBodyCombatSlotName(TEXT("FullBody_Combat"));
	static const FName DefaultCombatSlotName(TEXT("DefaultSlot"));

	/**
	 * 资源数组创建了但从未真正填写时，很容易出现这种“空占位结构体”。
	 * 这里把它们视为“并不是真正的连招数据”，
	 * 这样运行时才能安全回退。
	 */
	bool IsMeaningfulComboEntry(const FSIPAttackComboEntry& Entry)
	{
		return
			!Entry.EntryId.IsNone() ||
			Entry.WeaponModuleTag.IsValid() ||
			!Entry.Animation.IsNull() ||
			Entry.RequiredActionFamilyTag.IsValid() ||
			Entry.RequiredBodyStateTag.IsValid();
	}

	/**
	 * Rune Dagger 资产在几个迭代阶段之间经历过字段语义迁移：
	 * 1. 在缺失 FullBody_Combat 槽位的时期，资源曾被临时收口到 DefaultSlot。
	 * 2. 现在 FullBody_Combat 已重新成为显式全身战斗通道，因此 Rune Dagger 条目应回到 FullBody_Combat。
	 * 3. 少数旧条目的 NextComboIndex 指向了不存在的 ComboIndex=2 死胡同。
	 *
	 * 这里在资源加载时做最小修正，让“旧但有内容的资产”也能自动对齐当前真相源，
	 * 避免继续依赖历史 remap 和文档记忆去兜底。
	 */
	bool NormalizeLegacyRuneDaggerComboEntry(FSIPAttackComboEntry& Entry)
	{
		bool bChanged = false;

		// Keep authored combat slot names intact. Runtime slot remap now handles
		// `FullBody_Combat` -> `DefaultSlot` until the AnimBP owns that lane.
		if (Entry.SlotName.IsNone())
		{
			Entry.SlotName = LegacyFullBodyCombatSlotName;
			bChanged = true;
		}

		if (
			Entry.EntryId == TEXT("RuneDagger_Ice_RunAttack_02") &&
			Entry.NextComboIndex == 2)
		{
			Entry.NextComboIndex = 0;
			bChanged = true;
		}

		// 旧的 DriftTurn_Right 也曾落在同样的“ComboIndex=2 不存在”死胡同上。
		if (
			Entry.EntryId == TEXT("RuneDagger_Ice_DriftTurn_Right") &&
			Entry.NextComboIndex == 2)
		{
			Entry.NextComboIndex = 0;
			bChanged = true;
		}

		return bChanged;
	}
}

/**
 * 新建资产时也自动填充默认连招。
 */
void USIPAttackComboDataAsset::PostInitProperties()
{
	Super::PostInitProperties();
	AutoPopulateDefaultsIfNeeded();
}

/**
 * 已知 Rune Dagger 连招资产的加载后修复入口。
 */
void USIPAttackComboDataAsset::PostLoad()
{
	Super::PostLoad();
	AutoPopulateDefaultsIfNeeded();
}

/**
 * 第一版 Rune Dagger 连招包的中心真源。
 *
 * 运行时回退逻辑和资源自动补全都读这同一份列表，
 * 避免它们在不知不觉中各自漂移。
 */
const TArray<FSIPAttackComboEntry>& USIPAttackComboDataAsset::BuildRuneDaggerAttackComboEntriesV2()
{
	static const TArray<FSIPAttackComboEntry> Entries = []
	{
		TArray<FSIPAttackComboEntry> Result;

		FSIPAttackComboEntry Entry;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.SlotName = TEXT("FullBody_Combat");

		Entry.EntryId = TEXT("RuneDagger_Combo_01");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 1;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Combo_Attack_01_01_Seq.RTG_UEFN_AS_Combo_Attack_01_01_Seq")));
		Entry.HitWindowStartDelay = 0.42f;
		Entry.HitWindowEndDelay = 0.78f;
		Entry.Priority = 1;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Combo_02");
		Entry.ComboIndex = 1;
		Entry.NextComboIndex = 2;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Combo_Attack_02_01_Seq.RTG_UEFN_AS_Combo_Attack_02_01_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.46f;
		Entry.HitWindowEndDelay = 0.82f;
		Entry.Priority = 2;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_RunAttack_01");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 1;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Attack_01_Seq.RTG_UEFN_AS_Run_Attack_01_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.56f;
		Entry.HitWindowEndDelay = 0.96f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 260.0f;
		Entry.MaxGroundSpeed = 499.0f;
		Entry.MaxAbsTurnAngleDegrees = 29.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DriftSlash;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_DriftSlash;
		Entry.RequiredVariant = SIPCombatSemantic::VariantForward;
		Entry.Priority = 10;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_RunAttack_02");
		Entry.ComboIndex = 1;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Attack_02_Seq.RTG_UEFN_AS_Run_Attack_02_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.58f;
		Entry.HitWindowEndDelay = 1.00f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 260.0f;
		Entry.MaxAbsTurnAngleDegrees = 29.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DriftSlash;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_DriftSlash;
		Entry.RequiredVariant = SIPCombatSemantic::VariantForward;
		Entry.Priority = 11;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_DriftTurn_Left");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 1;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Parry_Counter_Attack_L_Seq.RTG_UEFN_AS_Parry_Counter_Attack_L_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.48f;
		Entry.HitWindowEndDelay = 0.90f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 260.0f;
		Entry.MinAbsTurnAngleDegrees = 30.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DriftTurnSlash;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_DriftTurn;
		Entry.RequiredVariant = SIPCombatSemantic::VariantLeft;
		Entry.RequiredTurnSign = 1;
		Entry.Priority = 20;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_DriftTurn_Right");
		Entry.ComboIndex = 1;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Parry_Counter_Attack_R_Seq.RTG_UEFN_AS_Parry_Counter_Attack_R_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.48f;
		Entry.HitWindowEndDelay = 0.90f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 260.0f;
		Entry.MinAbsTurnAngleDegrees = 30.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DriftTurnSlash;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_DriftTurn;
		Entry.RequiredVariant = SIPCombatSemantic::VariantRight;
		Entry.RequiredTurnSign = -1;
		Entry.Priority = 20;
		Result.Add(Entry);

		// --- SlideEntry: 冰面起手冲刺斩入 ---
		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_SlideEntry");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 1;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Start_Seq.RTG_UEFN_AS_Run_Combat_Fast_Start_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.38f;
		Entry.HitWindowEndDelay = 0.72f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 140.0f;
		Entry.MaxAbsTurnAngleDegrees = 30.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_SlideEntry;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_SlideEntry;
		Entry.RequiredVariant = SIPCombatSemantic::VariantForward;
		Entry.Priority = 15;
		Result.Add(Entry);

		// --- SlipRecovery Left: 冰面失衡左侧恢复 ---
		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_SlipRecovery_Left");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_L_Seq.RTG_UEFN_AS_Hit_Combat_L_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.30f;
		Entry.HitWindowEndDelay = 0.65f;
		Entry.bRequireIceSurface = true;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_SlipRecovery;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_SlipRecovery;
		Entry.RequiredVariant = SIPCombatSemantic::VariantLeft;
		Entry.Priority = 8;
		Result.Add(Entry);

		// --- SlipRecovery Right: 冰面失衡右侧恢复 ---
		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_SlipRecovery_Right");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_R_Seq.RTG_UEFN_AS_Hit_Combat_R_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.30f;
		Entry.HitWindowEndDelay = 0.65f;
		Entry.bRequireIceSurface = true;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_SlipRecovery;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_SlipRecovery;
		Entry.RequiredVariant = SIPCombatSemantic::VariantRight;
		Entry.Priority = 8;
		Result.Add(Entry);

		// --- DelayedRestart: 冰面短暂中断后的重新出发 ---
		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_DelayedRestart");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 1;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Dodge_Combat_F_0_Seq.RTG_UEFN_AS_Dodge_Combat_F_0_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.35f;
		Entry.HitWindowEndDelay = 0.68f;
		Entry.bRequireIceSurface = true;
		Entry.MinGroundSpeed = 140.0f;
		Entry.MaxAbsTurnAngleDegrees = 30.0f;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_DelayedRestart;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_DelayedRestart;
		Entry.RequiredVariant = SIPCombatSemantic::VariantForward;
		Entry.Priority = 12;
		Result.Add(Entry);

		// --- GlideExit: 冰面战斗退出滑行 ---
		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Ice_GlideExit");
		Entry.ComboIndex = 0;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Stop_Seq.RTG_UEFN_AS_Run_Combat_Fast_Stop_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.25f;
		Entry.HitWindowEndDelay = 0.55f;
		Entry.bRequireIceSurface = true;
		Entry.RequiredActionFamilyTag = SIPGameplayTags::State_Combat_ActionFamily_GlideExit;
		Entry.RequiredBodyStateTag = SIPGameplayTags::State_Combat_BodyState_GlideExit;
		Entry.Priority = 5;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Combo_03");
		Entry.ComboIndex = 2;
		Entry.NextComboIndex = 3;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/Attack/RTG_UEFN_AS_Combo_Attack_03_01_Seq.RTG_UEFN_AS_Combo_Attack_03_01_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.44f;
		Entry.HitWindowEndDelay = 0.80f;
		Entry.Priority = 3;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Combo_04");
		Entry.ComboIndex = 3;
		Entry.NextComboIndex = 4;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/Attack/RTG_UEFN_AS_Combo_Attack_04_01_Seq.RTG_UEFN_AS_Combo_Attack_04_01_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.46f;
		Entry.HitWindowEndDelay = 0.84f;
		Entry.Priority = 4;
		Result.Add(Entry);

		Entry = FSIPAttackComboEntry();
		Entry.EntryId = TEXT("RuneDagger_Combo_05");
		Entry.ComboIndex = 4;
		Entry.NextComboIndex = 0;
		Entry.WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		Entry.Animation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/Attack/RTG_UEFN_AS_Combo_Attack_05_01_Seq.RTG_UEFN_AS_Combo_Attack_05_01_Seq")));
		Entry.SlotName = TEXT("FullBody_Combat");
		Entry.HitWindowStartDelay = 0.50f;
		Entry.HitWindowEndDelay = 0.90f;
		Entry.Priority = 5;
		Result.Add(Entry);

		// NOTE: AirCombo / Launcher / Dodge 条目已暂时移除。
		// 原因：没有 bRequireAirborne 字段，空中条目 (P25-31) 在地面抢赢一切；
		//       冰面 Dodge (P18) 没有方向输入判定，也会劫持通用连击。
		// 等 FSIPAttackComboEntry 加入空中/闪避专用过滤字段后再恢复。

		return Result;
	}();

	return Entries;
}

/**
 * 只有当数组里至少存在一条真正写过语义或动画内容的条目时，才返回 true。
 */
bool USIPAttackComboDataAsset::HasMeaningfulComboEntries(const TArray<FSIPAttackComboEntry>& Entries)
{
	for (const FSIPAttackComboEntry& Entry : Entries)
	{
		if (IsMeaningfulComboEntry(Entry))
		{
			return true;
		}
	}

	return false;
}

/**
 * 当已知 Rune Dagger 连招资产加载出来的是空占位结构体而不是实际内容时，
 * 自动修复它。
 */
void USIPAttackComboDataAsset::AutoPopulateDefaultsIfNeeded()
{
	const bool bLooksLikeRuneDaggerAsset =
		DefaultWeaponModuleTag.MatchesTagExact(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) ||
		GetName().Contains(TEXT("RuneDagger"));
	if (!bLooksLikeRuneDaggerAsset)
	{
		return;
	}

	bool bChanged = false;

	if (!DefaultWeaponModuleTag.IsValid())
	{
		DefaultWeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
		bChanged = true;
	}

	if (!HasMeaningfulComboEntries(ComboEntries))
	{
		ComboEntries = BuildRuneDaggerAttackComboEntriesV2();
		bChanged = true;
	}
	else
	{
		// V1→V2 迁移：如果现有条目不包含 5 阶连击链的 Combo_03，
		// 说明数据资产序列化自 V1 时期，需要整体升级到 V2。
		bool bHasV2Chain = false;
		for (const FSIPAttackComboEntry& Entry : ComboEntries)
		{
			if (Entry.EntryId == TEXT("RuneDagger_Combo_03"))
			{
				bHasV2Chain = true;
				break;
			}
		}

		if (!bHasV2Chain)
		{
			ComboEntries = BuildRuneDaggerAttackComboEntriesV2();
			bChanged = true;
		}
		else
		{
			for (FSIPAttackComboEntry& Entry : ComboEntries)
			{
				bChanged |= NormalizeLegacyRuneDaggerComboEntry(Entry);
			}
		}
	}

#if WITH_EDITOR
	if (bChanged && GIsEditor)
	{
		MarkPackageDirty();
	}
#endif
}
