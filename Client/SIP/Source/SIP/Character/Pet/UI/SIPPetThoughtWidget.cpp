// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/UI/SIPPetThoughtWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> USIPPetThoughtWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void USIPPetThoughtWidget::SetThoughtText(const FString& Thought)
{
	if (ThoughtText)
	{
		const FString CleanThought = Thought.TrimStartAndEnd();
		ThoughtText->SetText(FText::FromString(CleanThought));

		const int32 WordCount = CountWords(CleanThought);
		const float BubbleHeight = WordCount > 14 ? 142.0f : (WordCount > 8 ? 118.0f : 96.0f);
		if (ThoughtBorderSlot)
		{
			ThoughtBorderSlot->SetSize(FVector2D(580.0f, BubbleHeight));
		}
		if (ThoughtTailSlot)
		{
			ThoughtTailSlot->SetPosition(FVector2D(0.0f, BubbleHeight * 0.5f - 2.0f));
		}
	}
}

int32 USIPPetThoughtWidget::CountWords(const FString& Text) const
{
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);
	return Words.Num();
}

void USIPPetThoughtWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ThoughtRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* TailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ThoughtTail"));
	TailBorder->SetBrushColor(FLinearColor(0.96f, 0.91f, 0.74f, 0.96f));
	RootCanvas->AddChild(TailBorder);

	if (UCanvasPanelSlot* TailSlot = Cast<UCanvasPanelSlot>(TailBorder->Slot))
	{
		ThoughtTailSlot = TailSlot;
		TailSlot->SetAnchors(FAnchors(0.5f, 0.73f, 0.5f, 0.73f));
		TailSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TailSlot->SetPosition(FVector2D(0.0f, 45.0f));
		TailSlot->SetSize(FVector2D(24.0f, 24.0f));
		TailSlot->SetAutoSize(false);
	}
	TailBorder->SetRenderTransformAngle(45.0f);

	UBorder* ThoughtBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ThoughtBorder"));
	ThoughtBorder->SetBrushColor(FLinearColor(0.96f, 0.91f, 0.74f, 0.96f));
	ThoughtBorder->SetPadding(FMargin(22.0f, 14.0f));
	RootCanvas->AddChild(ThoughtBorder);

	if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(ThoughtBorder->Slot))
	{
		ThoughtBorderSlot = BorderSlot;
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.73f, 0.5f, 0.73f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BorderSlot->SetSize(FVector2D(580.0f, 112.0f));
		BorderSlot->SetAutoSize(false);
	}

	ThoughtText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ThoughtText"));
	ThoughtText->SetText(FText::FromString(TEXT("...")));
	ThoughtText->SetAutoWrapText(true);
	ThoughtText->SetWrapTextAt(530.0f);
	ThoughtText->SetJustification(ETextJustify::Center);
	ThoughtText->SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.07f, 0.05f, 1.0f)));
	ThoughtText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 22));
	ThoughtBorder->SetContent(ThoughtText);
}
