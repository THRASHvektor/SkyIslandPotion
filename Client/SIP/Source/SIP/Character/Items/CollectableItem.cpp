#include "CollectableItem.h"
#include "AbilitySystemComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/CollectableHintWidget.h"

ACollectableItem::ACollectableItem(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    WidgetComponent->SetupAttachment(RootComponent);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    WidgetComponent->SetPivot(FVector2D(0.0f, 0.0f));
    WidgetComponent->SetWidget(nullptr);
    WidgetComponent->SetVisibility(false);
    // 该组合下，只有visible时才会tick
    WidgetComponent->SetTickMode(ETickMode::Automatic);
    WidgetComponent->SetComponentTickEnabled(true);
}

void ACollectableItem::BeginPlay()
{
    Super::BeginPlay();
}

void ACollectableItem::Destroyed()
{
    Super::Destroyed();
    WidgetComponent->SetWidget(nullptr);
}

void ACollectableItem::Interact(UAbilitySystemComponent* InteractorASC)
{
    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Interacted with %s"), *GetName()));
    Destroy();
}

FText ACollectableItem::GetInteractText() const
{
    return InteractText;
}

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

void ACollectableItem::OnEndFocus()
{
    WidgetComponent->SetVisibility(false);
}