#include "SIPComponent.h"

// SIP 基础组件目前不附加额外行为，但保留显式构造方便以后补默认配置。
//////////////////////////////////////////////////////////////////////////
// SIP 组件基类实现

USIPComponent::USIPComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

// 保留公共 BeginPlay 钩子，让派生组件都能复用同一个原生基类入口。
void USIPComponent::BeginPlay()
{
    // 先执行父类逻辑。
    Super::BeginPlay();
}
