# 战斗 + PCG 联动最小闭环方案

**日期**：2026-06-23
**背景**：团队 fallback，放弃 MM+PoseSearch 庞大愿景，做最小闭环交付（7月7号）
**创新点**：AIGC + PCG，核心是"一刀劈出 PCGGraph"

---

## 一、方向转变

### 放弃的
- MM + PoseSearch 的 6 家族冰面战斗语义解析
- 复杂的 AnimBP 动画桥 FullBody_Combat slot 联动
- PSD_IceCombat PoseSearch Database（UE5.4 API 不可用，卡 2 个月）

### 转向的
- GASP 移动兜底 + 简单战斗状态机（蒙太奇劫持）
- 战斗技能触发 PCG 生成/清除（"一刀劈出 PCGGraph"）
- 元素反应系统复用（已有 C++ 实现）
- AIGC 植物多 TAG（次要功能）

---

## 二、本地沉淀盘点（杀手级现成资产）

### 资产 1：投掷→PCG取消链（★★★★ 已跑通）

**这是最小闭环的现成骨架。** 完整链路：

```
GA_ThrowPotion → 蒙太奇 UpperBody_Cast → 弹丸飞行 → 命中 SphereTrace
→ ZoneActor.ReceiveElementHit(ElementTag) → ApplyReaction()
→ ClearPCGContent() (PCGComponent->CleanupLocal(true))
→ 或 GenerateVegetation() (Bloom 用新种子再生成)
```

**关键代码**：
- `SIPGameplayAbility_ThrowPotion.cpp:90` — 激活投掷
- `SIPPotionProjectile.cpp:66-114` — 命中结算 + 元素区域触发
- `SIPElementalZoneActor.cpp:220-229` — ClearPCGContent
- `SIPElementalZoneActor.cpp:126` — ApplyReaction（5 反应）

**为什么能绕开阻塞**：
- 投掷走 `UpperBody_Cast` slot，不走 `FullBody_Combat`（阻塞 #1）
- 不依赖 `PSD_IceCombat`（阻塞 #2）
- 不抑制 Motion Matching（`ShouldSuppressMotionMatching()` 只检查 `Attacking`，不检查 `Throwing`）

### 资产 2：元素反应系统（★★★ C++ 全实现）

5 个反应已全部实现，与需求逐字对齐：

| 区域元素 | + 输入元素 | → 反应 | 世界效果 |
|----------|-----------|--------|---------|
| Plant | Fire | Burn | 植被清除，路径开辟 |
| Ice | Fire | Melt | 冰结构溶解，隐藏区域暴露 |
| Heal | Ice | Freeze | 水面冻结，可行走平台 |
| Heal | Thunder | Electrify | 潮湿导电，AoE 眩晕 |
| Plant | Wind | Bloom | 孢子散播，新资源节点 |

**架构**：`SIPElementReactionSubsystem`（UWorldSubsystem）+ `ASIPElementalZoneActor`（关卡放置）+ 委托广播

### 资产 3：战斗法师动画（★★★ 充足）

Mage 资产包有大量施法/挥砍动画可直接用：
- `Combo_Attack_01/02/03` — 三连击
- `Run_Attack_01/02` — 跑动攻击
- `Skill_03/Skill_04` — 技能释放
- `Ultimate_Attack_Start` — 终极攻击
- `Aim_The_Target_Start/Loop` — 瞄准姿态
- `Buff` — 蓄力姿态

已重定向到 UEFN Mannequin 骨骼（`MageRetarget/IceRuneDagger/`）

### 资产 4：GASP 基座（★★ 移动稳）

- GASP 移动/穿越链稳定
- MotionWarping 可用于"劈砍→精准定位到 PCG 触发点"
- Chooser 的"上下文过滤搜索空间"范式可迁移到"战斗上下文过滤该触发哪个 PCGGraph"

### 资产 5：中期报告 §4.3.1（★★★ 学术护身符）

报告已立"玩家行为实时读写 PCG 世界数据"为核心创新，并论证了从"修改"到"创造"的跃迁路径。"一刀劈出 PCGGraph"是报告设想的具体化，学术叙事完整。

---

## 三、互联网调研发现

### 关键发现 1：Combat Fury（GASP 战斗集成方案）

**Combat Fury** 是一个 UE5 战斗系统（市场资产），社区已验证与 GASP 集成。

**Clydiie 频道**（用户提到的"Cly开头博主"）有完整教程系列：

| 视频 | 时长 | 价值 |
|------|------|------|
| The Ultimate Combat Fury Set Up Guide | 55:44 | **核心设置指南** |
| The Best Combat Tutorial Series - GASP | 30:11 | GASP 战斗入门 |
| Combat Fury 5.5 Update Is HUGE | 7:33 | 5.5 新特性 |
| Integrating Combat Fury into Story Framework | 27:32 | 框架集成 |
| ATC Project Part 1-8 | 18-76 min/集 | **完整实战系列** |
| Poise System - Custom Combat Fury Components | 13:46 | 躯干值系统 |
| Rage System - Custom Combat Fury Components | 8:25 | 怒气系统 |
| Integrate Advanced Traversal System To Combat Fury | 16:23 | 穿越系统集成 |

**ATC Project 系列**（9 集，从零到完整战斗）：
1. Replacing GASP Movement Set (76:06)
2. Replacing GASP Movement Set Part 2 (18:53)
3. Adding Combat Fury Animations (36:49)
4. Working On Combat Fury Enemy AI (33:10)
5. Changing Enemy Mesh & Weapons (20:36)
6. Modifying Combat Fury - Hit Spam Detection (26:08)
7. Adding Rage Mode To Combat Fury (30:25)
8. Improving Hit Spam Detection & Rage Mode (21:16)
9. Make Afterimage Dash Attack (14:29)

**其他 GASP 战斗教程作者**：
- **Last Save Studios** — GASP Combat System 系列（#47-#57，多集）
- **Zero2GameDev** — "GASP combo attacks combat system Motion Matching 5.5"
- **Wanli** — "Using Chooser & MotionMatching with Combat Fury"、"Thousand Hit Reactions"

### 关键发现 2：Runtime PCG（巫师4技术演示）

- "Runtime PCG in The Witcher 4 Unreal Engine Tech Demo" — Unreal Fest Stockholm 2025
- 证明 UE5 支持运行时 PCG 动态生成
- Unreal Fest 2023 "Introduction to PCG Workflows" — 官方 PCG 工作流

### 关键发现 3：Bilibili 中文资源

- "UE5.7的GASP运动匹配战斗系统教学" — 1849 播放，178 分钟
- "UE5 GASP演示：近战战斗系统" — 1460 播放
- "战斗教程系列 - 游戏动画示例 (GASP)" — 537 播放（Clydiie 翻译版）

### 关键发现 4：GitHub GASDocumentation

- `tranek/GASDocumentation` — 5838 stars，GAS 权威文档
- 直接对标项目的 GAS 实现

---

## 四、最小闭环方案

### 核心理念

**把投掷链的"命中→ZoneActor→PCG改写"模式，从弹丸命中移植到近战挥砍命中。**

```
玩家输入 → GA_MeleeSlash（新 GA）
→ 播放 Mage 挥砍蒙太奇（UpperBody_Cast 或 DefaultSlot）
→ Hit Window（AnimNotify 标记的命中帧）
→ 前方 SphereTrace（复用 SIPPotionProjectile::HandleImpact 模式）
→ 命中 ZoneActor → ReceiveElementHit(技能ElementTag, 命中点)
→ ApplyReaction() → ClearPCGContent() / GenerateVegetation()
→ 世界变化（植被清除/冰面冻结/孢子绽放）
```

### 三层架构

#### 第一层：战斗表现（GASP + 蒙太奇劫持）

- GASP 移动兜底（不动 GASP 的 MM 链）
- 战斗时用蒙太奇覆盖上半身（`UpperBody_Cast` slot，已验证）
- 用 Mage 的 `Combo_Attack_01/02/03` 做三连击
- 不需要 FullBody_Combat slot，不需要 PSD_IceCombat
- 可选：参考 Combat Fury 的做法（看 Clydiie 视频）

#### 第二层：命中→PCG 桥接（复用投掷链）

- 新建 `GA_MeleeSlash`（继承 `SIPGameplayAbility_ThrowPotion` 的命中模式）
- AnimNotify 标记 Hit Window
- Hit Window 期间做 SphereTrace
- 命中 `ASIPElementalZoneActor` → 调 `ReceiveElementHit()`
- **零改造复用元素反应系统**

#### 第三层：PCG 世界响应（复用已有）

- `ClearPCGContent()` — 植被清除（Burn/Melt/Freeze/Electrify）
- `GenerateVegetation()` — 植被再生成（Bloom，用新种子）
- `SpawnReactionActor()` — 生成冰平台/焦土等
- `PlayReactionVFX()` — Niagara 特效

### 交付清单（7月7号前）

| 优先级 | 任务 | 工作量 | 依赖 |
|--------|------|--------|------|
| P0 | GA_MeleeSlash：挥砍蒙太奇 + Hit Window + SphereTrace→ZoneActor | 2天 | 无 |
| P0 | 关卡放置测试 ZoneActor（Plant/Fire/Ice 各一个） | 0.5天 | 无 |
| P0 | 元素武器切换（给 GA_MeleeSlash 加 ElementTag 属性） | 0.5天 | P0 |
| P1 | 三连击 combo（Combo_Attack_01/02/03 循环） | 1天 | P0 |
| P1 | Bloom 反应演示（劈一刀→孢子绽放→新植被长出） | 0.5天 | P0 |
| P1 | Burn 反应演示（火元素劈砍→植被清除→路径开辟） | 0.5天 | P0 |
| P2 | AIGC 植物多 TAG（PCGGraph 插件魔改） | 2天 | P1 |
| P2 | 元素反应链（冰+火=融化，暴露隐藏区域） | 1天 | P1 |
| P3 | 战斗特效（Niagara 命中特效） | 1天 | P1 |
| P3 | 音效 | 0.5天 | P1 |

**总工作量**：P0-P1 = 5天，P2-P3 = 3.5天，合计 8.5天（7月7号前可完成）

### Demo 场景设计

```
玩家站在一片 PCG 生成的森林前
→ 切换火元素武器
→ 挥砍（Combo_Attack_01）
→ 前方森林被清除（Burn 反应），路径开辟
→ 切换风元素武器
→ 挥砍
→ 前方孢子绽放（Bloom 反应），新植被长出
→ 切换冰元素武器
→ 对水面挥砍
→ 水面冻结（Freeze 反应），形成可行走平台
→ 玩家走过去
```

**一句话演示**：一刀劈出 PCGGraph——劈砍改变世界。

---

## 五、风险与缓解

| 风险 | 缓解 |
|------|------|
| Mage 动画没有 hit notify 窗口 | 用 AnimNotify 帧手动标记，或用蒙太奇时间窗口 |
| UpperBody_Cast slot 只播上半身，不够"帅气" | 可用 DefaultSlot 全身播放（投掷链已验证 UpperBody_Cast，攻击链已验证 DefaultSlot remap） |
| Combat Fury 是付费资产 | 不买也行，用 Mage 动画 + 自建简单状态机即可；Combat Fury 只是参考 |
| PCG 运行时性能 | 已有投掷链验证过 CleanupLocal 性能可接受 |
| 元素反应系统是 POC 质量 | 足够 demo，交付前做一轮 bugfix |

---

## 六、调研视频清单（人工下载用）

### 优先级 P0（核心参考）
- [The Ultimate Combat Fury Set Up Guide](https://www.youtube.com/watch?v=crJM8oRoBxw) | Clydiie | 55:44
- [The Best Combat Tutorial Series - GASP](https://www.youtube.com/watch?v=XJDd7NQ_dqI) | Clydiie | 30:11
- [UE5 GASP + Mover: How to add Melee / Sword Combat](https://www.youtube.com/watch?v=k1gajVogcCM) | Last Save Studios | 1:00:25

### 优先级 P1（进阶参考）
- [ATC Project Part 3: Adding Combat Fury Animations](https://www.youtube.com/watch?v=ASqm0yHUxls) | Clydiie | 36:49
- [ATC Project Part 4: Working On Combat Fury Enemy AI](https://www.youtube.com/watch?v=vi78scxS_a8) | Clydiie | 33:10
- [Using Chooser & MotionMatching with Combat Fury](https://www.youtube.com/watch?v=ERKpFqZLPBA) | Wanli | 1:56
- [GASP combo attacks combat system Motion Matching 5.5](https://www.youtube.com/watch?v=f7yjI7LVxRs) | Zero2GameDev | 38:19

### 优先级 P2（PCG 参考）
- [Runtime PCG in The Witcher 4 Unreal Engine Tech Demo](https://www.youtube.com/watch?v=icIFFlOyob4) | Unreal Engine | 39:35
- [Introduction to PCG Workflows in UE5](https://www.youtube.com/watch?v=LMQDCEiLaQY) | Unreal Engine | 56:15
- [PCG: Introduction, Use Cases, and Production Best Practices](https://www.youtube.com/watch?v=TbNZ4GKaTow) | Unreal Fest 2025 | 39:37

### Bilibili 中文资源
- [UE5.7的GASP运动匹配战斗系统教学](https://www.bilibili.com/video/BV1D4rjBPEFY) | 178分钟
- [UE5 GASP演示：近战战斗系统](https://www.bilibili.com/video/BV1rtSUBwECt) | 1245分钟

---

## 七、与之前的对比

| 维度 | 之前（MM+PoseSearch） | 现在（战斗+PCG） |
|------|----------------------|------------------|
| 阻塞 | 2 个红色（slot + PSD） | 0 个（绕开了） |
| 工期 | 不可控（卡 2 个月） | 8.5 天可完成 |
| 创新 | MM+PoseSearch（技术深度高但交付难） | AIGC+PCG（学术叙事完整 + 可演示） |
| 复用 | 从头建战斗语义层 | 复用投掷链 + 元素反应系统 |
| 交付 | 无法 demo | 一刀劈出 PCGGraph |
| 风险 | 高（UE5.4 API 天花板） | 低（已验证链路） |

---

## 八、下一步

1. **你确认这个方案**（或调整优先级）
2. 我写详细实施计划（P0 的 GA_MeleeSlash 具体实现）
3. 开始写代码（GA_MeleeSlash + 关卡测试）
4. 你下载 Clydiie 的视频供参考
5. 7月7号前完成 P0-P1，P2-P3 视工期
