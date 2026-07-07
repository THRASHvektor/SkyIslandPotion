// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/SIPResourcePlantRecipe.h"

namespace
{
	TArray<ESIPResourcePlantSlot> GetAllSemanticSlots()
	{
		return {
			ESIPResourcePlantSlot::BodyShell,
			ESIPResourcePlantSlot::PrimarySilhouette,
			ESIPResourcePlantSlot::Core,
			ESIPResourcePlantSlot::OrbitSet
		};
	}

	FString SlotToDebugString(ESIPResourcePlantSlot Slot)
	{
		switch (Slot)
		{
		case ESIPResourcePlantSlot::BodyShell:
			return TEXT("BodyShell");
		case ESIPResourcePlantSlot::PrimarySilhouette:
			return TEXT("PrimarySilhouette");
		case ESIPResourcePlantSlot::Core:
			return TEXT("Core");
		case ESIPResourcePlantSlot::OrbitSet:
			return TEXT("OrbitSet");
		default:
			return TEXT("Unknown");
		}
	}

	void AddIssue(
		TArray<FSIPResourcePlantRecipeValidationIssue>& OutIssues,
		ESIPResourcePlantRecipeIssueSeverity Severity,
		ESIPResourcePlantRecipeIssueCode Code,
		ESIPResourcePlantSlot Slot,
		int32 CollapseLevel,
		const FString& Message)
	{
		FSIPResourcePlantRecipeValidationIssue Issue;
		Issue.Severity = Severity;
		Issue.Code = Code;
		Issue.Slot = Slot;
		Issue.CollapseLevel = CollapseLevel;
		Issue.Message = Message;
		OutIssues.Add(Issue);
	}
}

const FSIPResourcePlantMeshCollapseVariant* FSIPResourcePlantMeshSlotRecipe::FindCollapseVariant(int32 CollapseLevel) const
{
	const int32 SanitizedLevel = FMath::Clamp(CollapseLevel, 0, 3);
	return CollapseVariants.FindByPredicate([SanitizedLevel](const FSIPResourcePlantMeshCollapseVariant& Variant)
	{
		return FMath::Clamp(Variant.CollapseLevel, 0, 3) == SanitizedLevel;
	});
}

const FSIPResourcePlantMeshSlotRecipe* USIPResourcePlantRecipe::FindSlotRecipe(ESIPResourcePlantSlot Slot) const
{
	return MeshSlots.FindByPredicate([Slot](const FSIPResourcePlantMeshSlotRecipe& SlotRecipe)
	{
		return SlotRecipe.Slot == Slot;
	});
}

void USIPResourcePlantRecipe::ValidateRecipe(const FSIPResourcePlantRecipeValidationOptions& Options, TArray<FSIPResourcePlantRecipeValidationIssue>& OutIssues) const
{
	OutIssues.Reset();

	if (Options.bRequireAssemblyHints && AssemblyHint.SourceToUnrealScale <= 0.f)
	{
		AddIssue(
			OutIssues,
			ESIPResourcePlantRecipeIssueSeverity::Error,
			ESIPResourcePlantRecipeIssueCode::InvalidAssemblyScale,
			ESIPResourcePlantSlot::BodyShell,
			INDEX_NONE,
			TEXT("AssemblyHint.SourceToUnrealScale must be greater than zero."));
	}

	for (ESIPResourcePlantSlot Slot : GetAllSemanticSlots())
	{
		const FSIPResourcePlantMeshSlotRecipe* SlotRecipe = FindSlotRecipe(Slot);
		const int32 SlotCount = MeshSlots.FilterByPredicate([Slot](const FSIPResourcePlantMeshSlotRecipe& Candidate)
		{
			return Candidate.Slot == Slot;
		}).Num();

		if (SlotCount > 1)
		{
			AddIssue(
				OutIssues,
				ESIPResourcePlantRecipeIssueSeverity::Error,
				ESIPResourcePlantRecipeIssueCode::DuplicateSlot,
				Slot,
				INDEX_NONE,
				FString::Printf(TEXT("Slot %s is configured more than once."), *SlotToDebugString(Slot)));
		}

		if (!SlotRecipe)
		{
			if (Options.bRequireAllSemanticSlots)
			{
				AddIssue(
					OutIssues,
					ESIPResourcePlantRecipeIssueSeverity::Error,
					ESIPResourcePlantRecipeIssueCode::MissingSlot,
					Slot,
					INDEX_NONE,
					FString::Printf(TEXT("Missing mesh slot recipe for %s."), *SlotToDebugString(Slot)));
			}
			continue;
		}

		if (Options.bRequireAssemblyHints && !SIPResourcePlantAssembly::FindSlotHint(AssemblyHint, Slot))
		{
			AddIssue(
				OutIssues,
				ESIPResourcePlantRecipeIssueSeverity::Error,
				ESIPResourcePlantRecipeIssueCode::MissingAssemblyHint,
				Slot,
				INDEX_NONE,
				FString::Printf(TEXT("Missing assembly hint for %s."), *SlotToDebugString(Slot)));
		}

		TSet<int32> SeenCollapseLevels;
		for (const FSIPResourcePlantMeshCollapseVariant& Variant : SlotRecipe->CollapseVariants)
		{
			const int32 VariantLevel = FMath::Clamp(Variant.CollapseLevel, 0, 3);
			if (SeenCollapseLevels.Contains(VariantLevel))
			{
				AddIssue(
					OutIssues,
					ESIPResourcePlantRecipeIssueSeverity::Error,
					ESIPResourcePlantRecipeIssueCode::DuplicateCollapseVariant,
					Slot,
					VariantLevel,
					FString::Printf(TEXT("Slot %s has duplicate C%d variants."), *SlotToDebugString(Slot), VariantLevel));
			}
			SeenCollapseLevels.Add(VariantLevel);
		}

		if (Options.bRequireCollapseVariants)
		{
			for (int32 CollapseLevel = 0; CollapseLevel <= 3; ++CollapseLevel)
			{
				const FSIPResourcePlantMeshCollapseVariant* Variant = SlotRecipe->FindCollapseVariant(CollapseLevel);
				if (!Variant)
				{
					AddIssue(
						OutIssues,
						ESIPResourcePlantRecipeIssueSeverity::Error,
						ESIPResourcePlantRecipeIssueCode::MissingCollapseVariant,
						Slot,
						CollapseLevel,
						FString::Printf(TEXT("Slot %s is missing C%d variant."), *SlotToDebugString(Slot), CollapseLevel));
					continue;
				}

				if (Options.bRequireMeshes && Variant->Mesh.IsNull())
				{
					AddIssue(
						OutIssues,
						ESIPResourcePlantRecipeIssueSeverity::Error,
						ESIPResourcePlantRecipeIssueCode::MissingMesh,
						Slot,
						CollapseLevel,
						FString::Printf(TEXT("Slot %s C%d variant has no mesh."), *SlotToDebugString(Slot), CollapseLevel));
				}
			}
		}
		else if (Options.bRequireMeshes && SlotRecipe->Mesh.IsNull())
		{
			AddIssue(
				OutIssues,
				ESIPResourcePlantRecipeIssueSeverity::Error,
				ESIPResourcePlantRecipeIssueCode::MissingMesh,
				Slot,
				INDEX_NONE,
				FString::Printf(TEXT("Slot %s has no default mesh."), *SlotToDebugString(Slot)));
		}
	}
}

bool USIPResourcePlantRecipe::IsReadyForAssembly(const FSIPResourcePlantRecipeValidationOptions& Options) const
{
	TArray<FSIPResourcePlantRecipeValidationIssue> Issues;
	ValidateRecipe(Options, Issues);

	return !Issues.ContainsByPredicate([](const FSIPResourcePlantRecipeValidationIssue& Issue)
	{
		return Issue.Severity == ESIPResourcePlantRecipeIssueSeverity::Error;
	});
}

int32 USIPResourcePlantRecipe::CountIssues(const TArray<FSIPResourcePlantRecipeValidationIssue>& Issues, ESIPResourcePlantRecipeIssueCode Code)
{
	return Issues.FilterByPredicate([Code](const FSIPResourcePlantRecipeValidationIssue& Issue)
	{
		return Issue.Code == Code;
	}).Num();
}
