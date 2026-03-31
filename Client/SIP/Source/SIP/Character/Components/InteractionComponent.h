
#pragma once

#include "CoreMinimal.h"
#include "SIPComponent.h"
#include "../Interactable.h"
#include "InteractionComponent.generated.h"

/**
 * UInteractionComponent 
 * 用于搜寻可交互物体、处理玩家操控的角色与世界中可交互对象的交互逻辑（充当输入到交互对象内部逻辑的桥梁）
 * 针对ECC_Interactable碰撞通道进行查询
 */
UCLASS(config=Game)
class UInteractionComponent : public USIPComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent(const FObjectInitializer& ObjectInitializer);

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void InteractWithActor(AActor* TargetActor, UAbilitySystemComponent* OwnerASC);

protected:
    // 根据玩家朝向、物品权重等因素搜索可交互物体（继承IInteractable接口），并更新当前FocusedActor
    void SearchForFocusInteractable();

    // 评分公式，计算搜索分数选出最优交互对象
    virtual float CalculateSearchScore(AActor* Actor) const;

    // 绘制搜索范围的调试信息
    virtual void DrawSearchDebug() const;

    FVector GetViewLocation() const;
    FVector GetViewDirection() const;

    // 用于搜索可交互物体的搜索半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Search", meta = (hint = "搜索半径", ClampMin = "0.0"))
    float InteractableSearchRadius = 500.f;

    // 是否实时绘制搜索范围的调试信息
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Search", meta = (hint = "显示搜索范围"))
    bool bShowSearchDebug = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
    TScriptInterface<IInteractable> CurrentFocusedInteractable = nullptr;
};