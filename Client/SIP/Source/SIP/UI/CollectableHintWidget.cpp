#include "CollectableHintWidget.h"
#include "Components/TextBlock.h"

void UCollectableHintWidget::SetHintText(const FText& Text)
{
    if (HintText)
    {
        HintText->SetText(Text);
    }
}