#include "CoreMinimal.h"
#include "../Interactable.h"
#include "../SIPPawn.h"
#include "CollectableItem.generated.h"

class AbilitySystemComponent;
class UWidgetComponent;
class UCollectableHintWidget;

/**
 * Z 说明：
 * ACollectableItem 是游戏中可收集（拾取）物品的基类，继承自 ASIPPawn 和 IInteractable 接口
 * 需要手动地将mesh的Collision->ObjectType设置为 ECC_Interactable，才能被角色的交互组件检测到
 */

UCLASS()
class ACollectableItem : public ASIPPawn, public IInteractable
{
    GENERATED_BODY()

public:
    ACollectableItem(const FObjectInitializer& ObjectInitializer);

    virtual void Destroyed() override;

    // IInteractable Interface
    virtual void Interact(UAbilitySystemComponent* InteractorASC) override;
    virtual FText GetInteractText() const override;
    virtual void OnBeginFocus() override;
    virtual void OnEndFocus() override;

    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable", meta = (displayName = "物品交互文本"))
    FText InteractText;

    // 被focus时显示UI的组件
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
    TObjectPtr<UWidgetComponent> WidgetComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable", meta = (hint = "当该物体为玩家待交互物体时显示的提示UI"))
    TSubclassOf<UCollectableHintWidget> FocusedHintClass;
};