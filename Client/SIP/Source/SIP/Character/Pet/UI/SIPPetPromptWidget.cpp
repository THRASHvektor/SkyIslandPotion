// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/UI/SIPPetPromptWidget.h"

#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"
#include "Character/Pet/Components/SIPPetPromptSpawnComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"

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

FReply USIPPetPromptWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Tab && PromptSpawnComponent)
	{
		PromptSpawnComponent->HidePromptWidget();
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			PlayerController->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
	PanelBorder->SetBrushColor(FLinearColor(0.018f, 0.022f, 0.032f, 0.92f));
	RootCanvas->AddChild(PanelBorder);

	if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(PanelBorder->Slot))
	{
		BorderSlot->SetAnchors(FAnchors(0.78f, 0.5f, 0.78f, 0.5f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BorderSlot->SetPosition(FVector2D::ZeroVector);
		BorderSlot->SetSize(FVector2D(640.0f, 420.0f));
		BorderSlot->SetAutoSize(false);
	}

	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainBox"));
	PanelBorder->SetContent(MainBox);
	PanelBorder->SetPadding(FMargin(22.0f, 20.0f));

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Elemental Companion")));
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 28));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.56f)));
	TitleText->SetJustification(ETextJustify::Left);
	if (UVerticalBoxSlot* TitleSlot = MainBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
	}

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	SubtitleText->SetText(FText::FromString(TEXT("Shape a pet from element, species, and personality.")));
	SubtitleText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.78f, 0.86f, 0.92f)));
	if (UVerticalBoxSlot* SubtitleSlot = MainBox->AddChildToVerticalBox(SubtitleText))
	{
		SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	USizeBox* PromptSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptSizeBox"));
	PromptSizeBox->SetWidthOverride(596.0f);
	PromptSizeBox->SetHeightOverride(168.0f);

	PromptInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("PromptInput"));
	PromptInput->SetText(FText::GetEmpty());
	PromptInput->SetHintText(FText::FromString(TEXT("Describe the pet's element, species, and personality. Example: \"a brave thunder cat that loves fighting\" or \"a gentle water dragon that protects the player\".")));
	FTextBlockStyle PromptTextStyle = PromptInput->WidgetStyle.TextStyle;
	PromptTextStyle.SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 20));
	PromptTextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.04f, 0.05f, 0.065f, 1.0f)));
	PromptInput->SetTextStyle(PromptTextStyle);
	PromptInput->SetForegroundColor(FLinearColor(0.04f, 0.05f, 0.065f, 1.0f));
	PromptInput->SetAutoWrapText(true);
	PromptInput->SetWrapTextAt(560.0f);
	PromptSizeBox->SetContent(PromptInput);
	if (UVerticalBoxSlot* PromptSlot = MainBox->AddChildToVerticalBox(PromptSizeBox))
	{
		PromptSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		PromptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = MainBox->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	GenerateButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GenerateButton"));
	GenerateButton->SetBackgroundColor(FLinearColor(1.0f, 0.72f, 0.18f, 1.0f));
	GenerateButton->SetColorAndOpacity(FLinearColor(1.0f, 0.95f, 0.78f, 1.0f));
	USizeBox* GenerateButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GenerateButtonBox"));
	GenerateButtonBox->SetWidthOverride(150.0f);
	GenerateButtonBox->SetHeightOverride(42.0f);
	UTextBlock* GenerateLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GenerateLabel"));
	GenerateLabel->SetText(FText::FromString(TEXT("Generate")));
	GenerateLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17));
	GenerateLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.055f, 0.025f, 1.0f)));
	GenerateLabel->SetJustification(ETextJustify::Center);
	GenerateButtonBox->SetContent(GenerateLabel);
	GenerateButton->SetContent(GenerateButtonBox);
	GenerateButton->OnClicked.AddDynamic(this, &USIPPetPromptWidget::HandleGenerateClicked);
	if (UHorizontalBoxSlot* GenerateSlot = ButtonRow->AddChildToHorizontalBox(GenerateButton))
	{
		GenerateSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	ClearButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ClearButton"));
	ClearButton->SetBackgroundColor(FLinearColor(0.22f, 0.27f, 0.34f, 1.0f));
	ClearButton->SetColorAndOpacity(FLinearColor(0.85f, 0.90f, 0.98f, 1.0f));
	USizeBox* ClearButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ClearButtonBox"));
	ClearButtonBox->SetWidthOverride(112.0f);
	ClearButtonBox->SetHeightOverride(42.0f);
	UTextBlock* ClearLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClearLabel"));
	ClearLabel->SetText(FText::FromString(TEXT("Clear")));
	ClearLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 17));
	ClearLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 1.0f, 1.0f)));
	ClearLabel->SetJustification(ETextJustify::Center);
	ClearButtonBox->SetContent(ClearLabel);
	ClearButton->SetContent(ClearButtonBox);
	ClearButton->OnClicked.AddDynamic(this, &USIPPetPromptWidget::HandleClearClicked);
	if (UHorizontalBoxSlot* ClearSlot = ButtonRow->AddChildToHorizontalBox(ClearButton))
	{
		ClearSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("Ready")));
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 15));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.82f, 0.98f, 0.95f)));
	StatusText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* StatusSlot = MainBox->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 12.0f));
	}

	ResultJsonBox = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("ResultJsonBox"));
	ResultJsonBox->SetIsReadOnly(true);
	ResultJsonBox->SetText(FText::FromString(TEXT("")));
	ResultJsonBox->SetVisibility(PromptSpawnComponent && PromptSpawnComponent->bShowResultJsonInWidget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* ResultSlot = MainBox->AddChildToVerticalBox(ResultJsonBox))
	{
		ResultSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
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
