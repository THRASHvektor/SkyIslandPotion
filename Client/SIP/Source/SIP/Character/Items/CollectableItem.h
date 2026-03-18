#include "CoreMinimal.h"
#include "../Interactable.h"
#include "../SIPPawn.h"
#include "CollectableItem.generated.h"

class AbilitySystemComponent;
class UWidgetComponent;
class UCollectableHintWidget;

/**
 * Z 说明：
 * `ACollectableItem` 是游戏里可收集/可拾取物品的基类，
 * 继承自 `ASIPPawn` 并实现了 `IInteractable` 接口。
 * 物体的碰撞对象类型需要配置为 `ECC_Interactable`，这样交互组件才能检测到它。
 */
UCLASS()
class ACollectableItem : public ASIPPawn, public IInteractable
{
    GENERATED_BODY()

public:
    // 创建用于显示聚焦提示的 WidgetComponent。
    ACollectableItem(const FObjectInitializer& ObjectInitializer);

    // 在 Actor 销毁前清理临时 UI 状态，避免外部继续持有失效控件。
    virtual void Destroyed() override;

    // 交互接口 IInteractable：玩家确认交互时由交互系统调用。
    virtual void Interact(UAbilitySystemComponent* InteractorASC) override;
    // 返回聚焦时展示给 UI / Widget 的交互文本。
    virtual FText GetInteractText() const override;
    // 显示聚焦提示，并刷新当前文本。
    virtual void OnBeginFocus() override;
    // 失去聚焦时隐藏提示控件。
    virtual void OnEndFocus() override;

    // 为派生收集物保留的启动扩展点。
    virtual void BeginPlay() override;

protected:
    // 设计师配置的交互提示文本。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable", meta = (displayName = "鐗╁搧浜や簰鏂囨湰"))
    FText InteractText;

    // 聚焦时显示在屏幕上的提示控件组件。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
    TObjectPtr<UWidgetComponent> WidgetComponent;

    // 首次聚焦时实例化的提示 Widget 类。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable", meta = (hint = "褰撹鐗╀綋涓虹帺瀹跺緟浜や簰鐗╀綋鏃舵樉绀虹殑鎻愮ずUI"))
    TSubclassOf<UCollectableHintWidget> FocusedHintClass;
};
