#include "CollectableItem.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/CollectableHintWidget.h"

// 构造时一次性创建并配置屏幕空间提示控件。
ACollectableItem::ACollectableItem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    WidgetComponent->SetupAttachment(RootComponent);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComponent->SetPivot(FVector2D(0.0f, 0.0f));
    WidgetComponent->SetWidget(nullptr);
    WidgetComponent->SetVisibility(false);
    // 该模式下只有可见时才会参与 Tick。
    WidgetComponent->SetTickMode(ETickMode::Automatic);
    WidgetComponent->SetComponentTickEnabled(true);
}

// 保留默认 BeginPlay，方便收集物子类扩展启动逻辑而不改接口约定。
void ACollectableItem::BeginPlay()
{
    Super::BeginPlay();
}

// 显式断开 Widget 引用，避免聚焦 UI 继续指向已销毁物体。
void ACollectableItem::Destroyed()
{
    Super::Destroyed();
    WidgetComponent->SetWidget(nullptr);
}

// 默认交互先走接口链路，具体行为交给蓝图或派生类扩展。
void ACollectableItem::Interact(UAbilitySystemComponent* InteractorASC)
{
    IInteractable::Interact(InteractorASC);
    // GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Interacted with %s"), *GetName()));
    // Destroy();
}

// 向聚焦提示和交互预览暴露当前物品的交互文本。
FText ACollectableItem::GetInteractText() const
{
    return InteractText;
}

// 在物品被聚焦时按需创建提示控件，并同步刷新显示文本。
void ACollectableItem::OnBeginFocus()
{
    WidgetComponent->SetVisibility(true);
    WidgetComponent->SetComponentTickEnabled(true);
    
    if (WidgetComponent->GetWidget() == nullptr && FocusedHintClass)
    {
        WidgetComponent->SetWidgetClass(FocusedHintClass);
    }

    if(UCollectableHintWidget* HintWidget = Cast<UCollectableHintWidget>(WidgetComponent->GetWidget()))
    {
        HintWidget->SetHintText(InteractText);
    }
}

// 失去聚焦时隐藏控件即可，下次聚焦时可以继续复用同一个实例。
void ACollectableItem::OnEndFocus()
{
    WidgetComponent->SetVisibility(false);
}
