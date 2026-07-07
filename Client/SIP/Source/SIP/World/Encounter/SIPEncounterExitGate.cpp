#include "World/Encounter/SIPEncounterExitGate.h"

#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "SIPLogCategory.h"
#include "UI/CollectableHintWidget.h"

ASIPEncounterExitGate::ASIPEncounterExitGate()
{
	PrimaryActorTick.bCanEverTick = false;

	// AActor 默认没有 root，这里补一个供组件挂载。
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 与 ACollectableItem 对齐的屏幕空间提示控件。
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetPivot(FVector2D(0.0f, 0.0f));
	WidgetComponent->SetWidget(nullptr);
	WidgetComponent->SetVisibility(false);
	WidgetComponent->SetTickMode(ETickMode::Automatic);
	WidgetComponent->SetComponentTickEnabled(true);

	LockedInteractText = NSLOCTEXT("SIPEncounter", "EncounterExitLocked", "Defeat the sentry first");
	UnlockedInteractText = NSLOCTEXT("SIPEncounter", "EncounterExitUnlocked", "Leave encounter");
}

FText ASIPEncounterExitGate::GetInteractText() const
{
	return bIsUnlocked ? UnlockedInteractText : LockedInteractText;
}

void ASIPEncounterExitGate::Interact(UAbilitySystemComponent* InteractorASC)
{
	if (!bIsUnlocked)
	{
		UE_LOG(LogSIP, Log, TEXT("%s exit gate is still locked."), *GetName());
		return;
	}

	UE_LOG(LogSIP, Log, TEXT("%s exit gate used."), *GetName());
	// Route to the shared IInteractable BP hook so blueprints can implement OnInteract_BP.
	IInteractable::Interact(InteractorASC);

	// 解锁后的交互就是使用出口，立即请求地图切换。
	TravelToDestinationMap();
}

void ASIPEncounterExitGate::SetDestinationMap(const TSoftObjectPtr<UWorld>& InDestinationMap)
{
	DestinationMap = InDestinationMap;
}

void ASIPEncounterExitGate::TravelToDestinationMap_Implementation()
{
	if (DestinationMap.IsNull())
	{
		UE_LOG(LogSIP, Warning, TEXT("%s exit gate has no destination map assigned; travel skipped."), *GetName());
		K2_OnTravelToDestinationMap(false, NAME_None);
		return;
	}

	const FName DestinationLevelName(*FPackageName::ObjectPathToPackageName(DestinationMap.ToSoftObjectPath().ToString()));
	UE_LOG(LogSIP, Log, TEXT("%s exit gate travelling to map %s."), *GetName(), *DestinationLevelName.ToString());
	K2_OnTravelToDestinationMap(true, DestinationLevelName);

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DestinationMap, /*bAbsolute=*/true, TravelOptions);
}

// 聚焦时按需实例化 hint widget，并同步当前的锁定/解锁提示文本。
void ASIPEncounterExitGate::OnBeginFocus()
{
	bCurrentlyFocused = true;

	if (!WidgetComponent)
	{
		return;
	}

	WidgetComponent->SetVisibility(true);
	WidgetComponent->SetComponentTickEnabled(true);

	if (WidgetComponent->GetWidget() == nullptr && FocusedHintClass)
	{
		WidgetComponent->SetWidgetClass(FocusedHintClass);
	}

	RefreshHintWidgetText();
}

// 失去聚焦时隐藏控件，实例保留供下次聚焦复用。
void ASIPEncounterExitGate::OnEndFocus()
{
	bCurrentlyFocused = false;

	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(false);
	}
}

void ASIPEncounterExitGate::Destroyed()
{
	Super::Destroyed();
	if (WidgetComponent)
	{
		WidgetComponent->SetWidget(nullptr);
	}
}

void ASIPEncounterExitGate::SetGateUnlocked(bool bUnlocked)
{
	if (bIsUnlocked == bUnlocked)
	{
		return;
	}

	bIsUnlocked = bUnlocked;
	K2_OnGateUnlockedChanged(bIsUnlocked);

	// 玩家可能正对着门时门被解锁，提示文本要立刻刷新。
	if (bCurrentlyFocused)
	{
		RefreshHintWidgetText();
	}

	UE_LOG(LogSIP, Log, TEXT("%s exit gate unlocked state changed: %s."), *GetName(), bIsUnlocked ? TEXT("true") : TEXT("false"));
}

void ASIPEncounterExitGate::RefreshHintWidgetText()
{
	if (!WidgetComponent)
	{
		return;
	}

	if (UCollectableHintWidget* HintWidget = Cast<UCollectableHintWidget>(WidgetComponent->GetWidget()))
	{
		HintWidget->SetHintText(GetInteractText());
	}
}
