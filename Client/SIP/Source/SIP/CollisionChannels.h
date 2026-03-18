#pragma once

/**
 * Z 说明：
 * 项目内自定义碰撞通道定义，对应关系配置在 `Config/DefaultEngine.ini`。
 */
// 交互检测专用的 Trace/Object Channel，用来命中可收集物和其他可交互对象。
#define ECC_Interactable ECC_GameTraceChannel1
