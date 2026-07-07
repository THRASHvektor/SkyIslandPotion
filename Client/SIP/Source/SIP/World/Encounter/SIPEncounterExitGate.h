#pragma once

#include "CoreMinimal.h"
#include "Character/Interactable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "SIPEncounterExitGate.generated.h"

class UAbilitySystemComponent;
class UWidgetComponent;
class UCollectableHintWidget;

UCLASS(Blueprintable)
class SIP_API ASIPEncounterExitGate : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASIPEncounterExitGate();

	virtual FText GetInteractText() const override;
	virtual void Interact(UAbilitySystemComponent* InteractorASC) override;

	// 聚焦提示显隐，行为对齐 ACollectableItem。
	virtual void OnBeginFocus() override;
	virtual void OnEndFocus() override;

	// 销毁时断开 Widget 引用，避免聚焦 UI 继续指向已销毁物体。
	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Encounter Exit")
	void SetGateUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintPure, Category = "SIP|Encounter Exit")
	bool IsGateUnlocked() const { return bIsUnlocked; }

	// 设置玩家通过此出口要前往的下一张地图（由 EncounterPCG 生成时写入）。
	UFUNCTION(BlueprintCallable, Category = "SIP|Encounter Exit")
	void SetDestinationMap(const TSoftObjectPtr<UWorld>& InDestinationMap);

	UFUNCTION(BlueprintPure, Category = "SIP|Encounter Exit")
	const TSoftObjectPtr<UWorld>& GetDestinationMap() const { return DestinationMap; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit|Text")
	FText LockedInteractText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit|Text")
	FText UnlockedInteractText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit")
	bool bIsUnlocked = false;

	// 玩家使用此出口后要前往的下一张地图（可由 PCG 组件在生成时按 Actor/Pattern 覆盖顺序写入）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> DestinationMap;

	// 触发跳图时透传给 UGameplayStatics::OpenLevel 的可选 Options 字符串。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit")
	FString TravelOptions;

	// 聚焦时显示在屏幕上的提示控件组件。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	// 首次聚焦时实例化的提示 Widget 类。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Encounter Exit")
	TSubclassOf<UCollectableHintWidget> FocusedHintClass;

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Encounter Exit", DisplayName = "On Gate Unlocked Changed")
	void K2_OnGateUnlockedChanged(bool bUnlocked);

	// 玩家触发出口即将执行地图切换前的蓝图钩子，可用于淡出/存档等。
	// 若 bWillTravel 为 false，说明目的地无效，交互被视作空操作。
	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Encounter Exit", DisplayName = "On Travel To Destination Map")
	void K2_OnTravelToDestinationMap(bool bWillTravel, FName DestinationMapName);

	// 真正执行地图切换的入口，可被子类/蓝图重写来接管跳图逻辑。
	UFUNCTION(BlueprintNativeEvent, Category = "SIP|Encounter Exit")
	void TravelToDestinationMap();
	virtual void TravelToDestinationMap_Implementation();

private:
	// 记录当前是否处于聚焦态，用于在锁定状态切换时刷新提示文本。
	bool bCurrentlyFocused = false;

	// 将当前应显示的交互文本同步到已实例化的 hint widget。
	void RefreshHintWidgetText();
};
