## Unreal Engine control

**soft-ue-cli hybrid** — Python CLI v1.37.0, Bridge Plugin: Runtime v1.27.0 + Editor v1.20.6 (optimized for UE 5.4)
Installed at: `Client/SIP/soft-ue-cli/`, Plugin at: `Client/SIP/Plugins/SoftUEBridge/`

`C:\Users\zzg\AppData\Local\Programs\Python\Python311\python.exe -m soft_ue_cli` controls this UE project via the SoftUEBridge plugin.
Run `C:\Users\zzg\AppData\Local\Programs\Python\Python311\python.exe -m soft_ue_cli --help` to see all available commands.
The game or editor must be running with SoftUEBridge enabled before using UE commands.

### Key new Runtime tools (post-rebuild)
- `inspect-anim-instance` — Snapshot AnimInstance: state machines, montages, slots, notifies, blend weights
- `trigger-input` — Simulate player input (key/action/move-to/look-at) for automated testing
- `get-property` — Read actor/component property values via reflection
- `capture` — Screenshot viewport/PIE
- `batch-call` — Dispatch multiple bridge calls in one HTTP request

### Key new Editor tools (post-rebuild)
- `compile-blueprint` — Compile BP/AnimBP programmatically
- `insert-graph-node` — Atomically insert node between connected nodes
- `set-node-property` — Set graph node properties
- `build-and-relaunch` — C++ rebuild + relaunch editor
- `trigger-live-coding` — Hot reload C++ changes
- `save-asset` — Save assets

After the user rebuilds and launches UE, verify with:
  C:\Users\zzg\AppData\Local\Programs\Python\Python311\python.exe -m soft_ue_cli check-setup

---

## Project Identity

**Sky Island Potion** — 语义驱动的 3C 动作 RPG 原型
Unreal Engine 5.4.4 · GAS · GASP Motion Matching · PCG · C++

**核心公式：** `ActionStyle = f(Env, Weapon, Traversal, BodyState)`
**金色路径：** Ice + RuneDagger 语义战斗（6 个动作家族，13 个连击条目）
**当前分支：** `feature/combat-system`

---

## Document Structure

> 整理日期：2026-06-16。所有文档按类型分入 6 个子文件夹，非 SIP 内容单独归档。

```
Client/SIP/Docs/
├── 01_Core_Design/             # 核心设计文档
│   ├── Design.md               #   三层语义体表达模型核心设计
│   ├── OpusDesign.md           #   Opus 可行性分析、硬约束、分阶段路线图
│   └── Codebase_RAG_Index.md   #   RAG 检索索引（推荐阅读顺序、源码模块索引）
│
├── 02_Combat_System/           # 战斗系统设计与分析
│   ├── CombatSemantic_GoldenPath_IceRuneDagger.md    #   冰面符文匕首金色路径可执行规格
│   ├── CombatSemantic_DevNotes_2026-03-30.md         #   开发笔记（已知问题记录）
│   ├── CombatSemantic_Sprint2_Notes.md               #   Sprint 2 笔记
│   ├── CombatAnimation_FullChain_DeepDive_2026-04-18.md  #   战斗动画全链路分析
│   ├── CombatAnimationSystem_Architecture_Implementation_zh_2026-04-19.md  #   中文架构与实现
│   └── GASP_ActionSystem_FullChain_Reference_2026-04-18.md  #   GASP 动作系统全链路参考
│
├── 03_Asset_Pipeline/          # 资产创建、重定向、AnimBP 连线
│   ├── MageAsset_SemanticAdaptation.md               #   Mage 动画包语义适配方案
│   ├── MageRetarget_Workflow_zh.md                   #   重定向工作流
│   ├── ABP_SandboxCharacter_MM_Combat_Wiring_Guide_2026-04-11.md  #   AnimBP 手动连线 6 步指南
│   ├── Project_LiveExtraction_2026-04-11.md           #   设计-代码-资产三方核验
│   ├── MageAsset_SemanticManifest.json                #   Mage 资产语义清单
│   └── MageRetarget_IceRuneDagger_Manifest.json       #   冰符文匕首重定向清单
│
├── 04_Opus_Execution_Logs/     # Opus 自动化执行记录（24 个 flow + 路线图）
│   ├── Opus_Optimization_Roadmap_2026-04-11.md        #   Opus 诊断的 Phase 0-4 路线图
│   ├── Opus_Flow_01 → Opus_Flow_09                    #   PoseSearch Tag / PSD 路由 / Mage 重定向
│   ├── Opus_Flow_10_*  (12 files)                     #   Combat Combo V2 / Slot Fix / Hotfix 系列
│   ├── Opus_Flow_11_*                                  #   Montage MM Transition SnapBack Fix
│   ├── Opus_Hotfix_IceSpeedOverride_Verification_2026-04-12.md
│   ├── Content_FileManifest.json / Source_FileManifest.json
│   └── Opus_Flow_09_MageBatchRetarget_Report_2026-04-12.json
│
├── 05_Tools_and_RAG/           # 工具文档与 RAG 配置
│   ├── SoftUECLI_QuickStart.md #   soft-ue-cli 快速入门
│   ├── ue_rag_prompt_system.md #   RAG 生成 prompt 系统
│   └── ue_rag_quick_reference.md  #   RAG 快速参考
│
├── 06_Presentation/            # 面试与答辩准备
│   └── 面试拷打手册_源码版_2026-04-21.md              #   15 模块 Q&A 源码级面试准备
│
└── 工程说明.txt                # 首次项目设置（VS 工程、MSVC 工具链版本）
```

### 项目根目录其他文件夹

```
External/JBT_Wujie/             # 吉比特无界计划 24h 竞赛（湮岛提案）—— 非 SIP 内容
Reports/                        # 学术报告
│   └── 2_PDFsam_INterInterim Report.pdf   # 中期报告（44 页，6 章）
diagrams/                       # LaTeX 图表源文件 + 编译 PDF + PNG
│   ├── fig_three_semantic_model.*    # 三层语义模型图
│   ├── fig_combat_pipeline.*         # 战斗管线图
│   └── fig_narrative_loop.*          # 叙事循环图
Scripts/                        # 根级工具脚本
│   ├── bridge/                 #   SoftUEBridge 查询/运行/分离工具
│   ├── _flow04-09_*.py         #   Opus Flow Python 自动化脚本
│   └── _post_rebuild_semantic_profile.py
Client/SIP/Scripts/             # SIP 级工具脚本
│   ├── wander_ai.py
│   └── _post_rebuild_*.py
```

---

## 关键文件速查

| 想知道什么 | 看哪个 |
|-----------|--------|
| 战斗怎么设计的 | `Docs/02_Combat_System/CombatSemantic_GoldenPath_IceRuneDagger.md` |
| 为什么卡在 AnimBP | `Docs/03_Asset_Pipeline/ABP_SandboxCharacter_MM_Combat_Wiring_Guide_2026-04-11.md` |
| 代码架构怎么样 | `Docs/01_Core_Design/Design.md` |
| Opus 做了什么 | `Docs/04_Opus_Execution_Logs/Opus_Optimization_Roadmap_2026-04-11.md` |
| 中期报告画的什么饼 | `../../Reports/2_PDFsam_INterInterim Report.pdf` |
| 面试怎么讲 | `Docs/06_Presentation/面试拷打手册_源码版_2026-04-21.md` |
| UE 怎么控制 | `Docs/05_Tools_and_RAG/SoftUECLI_QuickStart.md` |
| 无界计划是什么 | `../../External/JBT_Wujie/00_README_提交说明.md` |
