#!/usr/bin/env python3
"""
创建 PSD_IceCombat PoseSearch Database。

复用 _flow08_create_pss_ice.py 的模式：
  1. 复制 PSD_IceLocomotion 作为基础（继承 Ice schema）
  2. 清除 locomotion 动画
  3. 添加 21 个战斗动画（来自 MageRetarget_IceRuneDagger_Manifest.json）

通过 SoftUEBridge 的 run-python-script 执行：
  py -3 -m soft_ue_cli run-python-script --file Scripts/_create_psd_ice_combat.py
"""

import unreal
import json

# === 配置 ===
SOURCE_DB = "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/PSD_IceLocomotion"
TARGET_DB = "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/PSD_IceCombat"
ICE_SCHEMA = "/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Ice"

# 21 个战斗动画（来自 MageRetarget_IceRuneDagger_Manifest.json）
COMBAT_ANIMATIONS = [
    # 连击攻击
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Combo_Attack_01_01_Seq",
    # 跑动攻击
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Attack_01_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Attack_02_Seq",
    # 格挡反击
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Parry_Counter_Attack_L_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Parry_Counter_Attack_R_Seq",
    # 受击反应
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_L_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_R_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_Large_L_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Hit_Combat_Large_R_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Block_Hit_Break_Seq",
    # 闪避
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Dodge_Combat_F_0_Seq",
    # 战斗跑动 start/stop/turn
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Start_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Stop_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Turn_L_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Fast_Turn_R_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Start_Fast_L_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_Start_Fast_R_Seq",
    # 战斗跑动 → 跑动/行走过渡
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Run_Combat_to_Run_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Walk_Combat_to_Walk_Seq",
    # 战斗转向
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Turn_Combat_L_90_Seq",
    "/Game/Characters/UEFN_Mannequin/Animations/MageRetarget/IceRuneDagger/RTG_UEFN_AS_Turn_Combat_R_90_Seq",
]


def main():
    result = {
        "source_db": SOURCE_DB,
        "target_db": TARGET_DB,
        "ice_schema": ICE_SCHEMA,
        "combat_animations_count": len(COMBAT_ANIMATIONS),
        "steps": [],
    }

    # 步骤 1: 创建 PSD_IceCombat（复制 IceLocomotion 作为基础）
    if unreal.EditorAssetLibrary.does_asset_exist(TARGET_DB):
        result["steps"].append({"step": 1, "action": "exists", "status": "skip"})
        db = unreal.EditorAssetLibrary.load_asset(TARGET_DB)
    else:
        created = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_DB, TARGET_DB)
        if not created:
            raise RuntimeError(f"Failed to duplicate {SOURCE_DB} -> {TARGET_DB}")
        db = unreal.EditorAssetLibrary.load_asset(TARGET_DB)
        result["steps"].append({"step": 1, "action": "duplicated", "status": "ok"})

    if not db:
        raise RuntimeError(f"Failed to load {TARGET_DB}")

    # 步骤 2: 确认 schema 是 PSS_Ice
    schema = db.get_editor_property("schema")
    schema_path = schema.get_path_name() if schema else None
    result["steps"].append({"step": 2, "action": "verify_schema", "schema": schema_path})

    if schema_path != ICE_SCHEMA:
        ice_schema_asset = unreal.EditorAssetLibrary.load_asset(ICE_SCHEMA)
        if ice_schema_asset:
            db.set_editor_property("schema", ice_schema_asset)
            unreal.EditorAssetLibrary.save_loaded_asset(db)
            result["steps"].append({"step": 2, "action": "set_schema_to_ice", "status": "ok"})

    # 步骤 3: TODO — 清除从 IceLocomotion 继承的 locomotion 动画
    # 需要探查 UPoseSearchDatabase 的编辑器 API:
    #   - 如何列举数据库里的动画序列
    #   - 如何移除动画序列
    # 建议先用 soft-ue-cli 探查:
    #   py -3 -m soft_ue_cli asset query --asset-path <TARGET_DB> --include-properties
    result["steps"].append({
        "step": 3,
        "action": "clear_locomotion_anims",
        "status": "TODO — 需要探查 UPoseSearchDatabase 编辑器 API"
    })

    # 步骤 4: TODO — 添加 21 个战斗动画
    # 需要探查 UPoseSearchDatabase 如何添加动画序列:
    #   - 可能是通过 db.get_editor_property("anim_sequences") 修改数组
    #   - 或通过 db.add_animation(seq) 之类的方法
    # 动画列表已在 COMBAT_ANIMATIONS 里准备好
    result["steps"].append({
        "step": 4,
        "action": "add_combat_anims",
        "status": "TODO — 需要探查 UPoseSearchDatabase 编辑器 API",
        "animation_count": len(COMBAT_ANIMATIONS),
        "animation_list": COMBAT_ANIMATIONS
    })

    # 步骤 5: 保存
    unreal.EditorAssetLibrary.save_loaded_asset(db)
    result["steps"].append({"step": 5, "action": "save", "status": "ok"})

    print(json.dumps(result, ensure_ascii=False, default=str, indent=2))


if __name__ == "__main__":
    main()
