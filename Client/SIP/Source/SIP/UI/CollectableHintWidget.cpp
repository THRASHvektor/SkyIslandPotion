#include "CollectableHintWidget.h"
#include "Components/TextBlock.h"

// 保护半初始化状态下的控件，避免外部在绑定完成前设置文本时报错。
void UCollectableHintWidget::SetHintText(const FText& Text)
{
    if (HintText)
    {
        HintText->SetText(Text);
    }
}
