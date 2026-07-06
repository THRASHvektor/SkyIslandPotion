// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SIPPetPromptWidget.h"

#include "Character/Components/SIPPetPersonalityJsonComponent.h"
#include "Character/Components/SIPPetPromptSpawnComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"

TSharedRef<SWidget> USIPPetPromptWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void USIPPetPromptWidget::InitializePromptWidget(USIPPetPromptSpawnComponent* InPromptSpawnComponent)
{
	PromptSpawnComponent = InPromptSpawnComponent;
	BindPromptSpawnComponent();
}

void USIPPetPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindPromptSpawnComponent();
}

void USIPPetPromptWidget::NativeDestruct()
{
	if (GenerateButton)
	{
		GenerateButton->OnClicked.RemoveDynamic(this, &USIPPetPromptWidget::HandleGenerateClicked);
	}

	if (ClearButton)
	{
		ClearButton->OnClicked.RemoveDynamic(this, &USIPPetPromptWidget::HandleClearClicked);
	}

	if (PromptSpawnComponent)
	{
		PromptSpawnComponent->OnPromptPetSpawned.RemoveDynamic(this, &USIPPetPromptWidget::HandlePromptPetSpawned);
	}

	if (ActivePersonalityComponent)
	{
		ActivePersonalityComponent->OnPersonalityJsonGenerated.RemoveDynamic(this, &USIPPetPromptWidget::HandlePersonalityJsonGenerated);
	}

	Super::NativeDestruct();
}

void USIPPetPromptWidget::SetStatusText(const FString& InStatus)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(InStatus));
	}
}

void USIPPetPromptWidget::SetResultJson(bool bSuccess, const FString& JsonString)
{
	if (ResultJsonBox)
	{
		ResultJsonBox->SetText(FText::FromString(JsonString));
	}

	SetStatusText(bSuccess ? TEXT("Pet Generated") : TEXT("Pet Generated (Local Fallback)"));
}

void USIPPetPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
	RootCanvas->AddChild(PanelBorder);

	if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(PanelBorder->Slot))
	{
		BorderSlot->SetPosition(FVector2D(40.0f, 80.0f));
		BorderSlot->SetSize(FVector2D(660.0f, 430.0f));
		BorderSlot->SetAutoSize(false);
	}

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainBox"));
	PanelBorder->SetContent(MainBox);
	PanelBorder->SetPadding(FMargin(14.0f));

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("AI Pet Generator")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 1.0f, 0.85f)));
	TitleText->SetJustification(ETextJustify::Left);
	MainBox->AddChildToVerticalBox(TitleText);

	PromptInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("PromptInput"));
	PromptInput->SetText(FText::FromString(TEXT("一只风属性、好奇、喜欢探索、会主动帮玩家过悬崖的宠物")));
	PromptInput->SetHintText(FText::FromString(TEXT("Describe your pet...")));
	if (UVerticalBoxSlot* PromptSlot = MainBox->AddChildToVerticalBox(PromptInput))
	{
		PromptSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 8.0f));
		PromptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = MainBox->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	GenerateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GenerateButton"));
	UTextBlock* GenerateLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GenerateLabel"));
	GenerateLabel->SetText(FText::FromString(TEXT("Generate Pet")));
	GenerateButton->SetContent(GenerateLabel);
	GenerateButton->OnClicked.AddDynamic(this, &USIPPetPromptWidget::HandleGenerateClicked);
	if (UHorizontalBoxSlot* GenerateSlot = ButtonRow->AddChildToHorizontalBox(GenerateButton))
	{
		GenerateSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	ClearButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ClearButton"));
	UTextBlock* ClearLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClearLabel"));
	ClearLabel->SetText(FText::FromString(TEXT("Clear")));
	ClearButton->SetContent(ClearLabel);
	ClearButton->OnClicked.AddDynamic(this, &USIPPetPromptWidget::HandleClearClicked);
	ButtonRow->AddChildToHorizontalBox(ClearButton);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("Ready")));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.9f, 1.0f)));
	if (UVerticalBoxSlot* StatusSlot = MainBox->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	ResultJsonBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("ResultJsonBox"));
	ResultJsonBox->SetIsReadOnly(true);
	ResultJsonBox->SetText(FText::FromString(TEXT("")));
	ResultJsonBox->SetVisibility(PromptSpawnComponent && PromptSpawnComponent->bShowResultJsonInWidget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* ResultSlot = MainBox->AddChildToVerticalBox(ResultJsonBox))
	{
		ResultSlot->SetPadding(FMargin(0.0f));
		ResultSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void USIPPetPromptWidget::BindPromptSpawnComponent()
{
	if (!PromptSpawnComponent)
	{
		return;
	}

	if (!PromptSpawnComponent->OnPromptPetSpawned.IsAlreadyBound(this, &USIPPetPromptWidget::HandlePromptPetSpawned))
	{
		PromptSpawnComponent->OnPromptPetSpawned.AddDynamic(this, &USIPPetPromptWidget::HandlePromptPetSpawned);
	}
}

void USIPPetPromptWidget::HandleGenerateClicked()
{
	if (!PromptSpawnComponent || !PromptInput)
	{
		SetStatusText(TEXT("Missing prompt spawn component."));
		return;
	}

	const FString Prompt = PromptInput->GetText().ToString();
	if (Prompt.TrimStartAndEnd().IsEmpty())
	{
		SetStatusText(TEXT("Please enter a pet description."));
		return;
	}

	SetStatusText(TEXT("Generating..."));
	if (ResultJsonBox)
	{
		ResultJsonBox->SetText(FText::FromString(TEXT("")));
	}

	PromptSpawnComponent->SpawnPetFromPrompt(Prompt);
}

void USIPPetPromptWidget::HandleClearClicked()
{
	if (PromptInput)
	{
		PromptInput->SetText(FText::GetEmpty());
	}

	if (ResultJsonBox)
	{
		ResultJsonBox->SetText(FText::GetEmpty());
	}

	SetStatusText(TEXT("Ready"));
}

void USIPPetPromptWidget::HandlePromptPetSpawned(AActor* SpawnedPet, USIPPetPersonalityJsonComponent* PersonalityComponent)
{
	if (ActivePersonalityComponent)
	{
		ActivePersonalityComponent->OnPersonalityJsonGenerated.RemoveDynamic(this, &USIPPetPromptWidget::HandlePersonalityJsonGenerated);
	}

	ActivePersonalityComponent = PersonalityComponent;
	if (ActivePersonalityComponent)
	{
		if (!ActivePersonalityComponent->OnPersonalityJsonGenerated.IsAlreadyBound(this, &USIPPetPromptWidget::HandlePersonalityJsonGenerated))
		{
			ActivePersonalityComponent->OnPersonalityJsonGenerated.AddDynamic(this, &USIPPetPromptWidget::HandlePersonalityJsonGenerated);
		}
	}

	SetStatusText(SpawnedPet ? TEXT("Pet spawned, waiting for JSON...") : TEXT("Pet spawn failed."));
}

void USIPPetPromptWidget::HandlePersonalityJsonGenerated(bool bSuccess, const FString& JsonString)
{
	SetResultJson(bSuccess, JsonString);
}
