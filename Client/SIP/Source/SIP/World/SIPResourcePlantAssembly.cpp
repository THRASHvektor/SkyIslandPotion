// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPResourcePlantAssembly.h"

namespace
{
	constexpr float BodyShellWeight = 0.20f;
	constexpr float PrimarySilhouetteWeight = 0.35f;
	constexpr float CoreWeight = 0.25f;
	constexpr float OrbitSetWeight = 0.20f;

	struct FCollapseBandProfile
	{
		float TargetSeverity = 0.f;
		float TargetDispersion = 0.f;
		float Temperature = 0.1f;
	};

	float Square(float Value)
	{
		return Value * Value;
	}

	FCollapseBandProfile GetBandProfile(ESIPResourcePlantCollapseBand Band)
	{
		switch (Band)
		{
		case ESIPResourcePlantCollapseBand::Stable_C0:
			return { 0.25f, 0.05f, 0.15f };
		case ESIPResourcePlantCollapseBand::Weathered_C1:
			return { 0.95f, 0.18f, 0.35f };
		case ESIPResourcePlantCollapseBand::Fractured_C2:
			return { 1.80f, 0.38f, 0.65f };
		case ESIPResourcePlantCollapseBand::Unstable_C3:
			return { 2.45f, 0.58f, 1.00f };
		default:
			return { 0.95f, 0.18f, 0.35f };
		}
	}

	int32 GetBandSalt(ESIPResourcePlantCollapseBand Band)
	{
		switch (Band)
		{
		case ESIPResourcePlantCollapseBand::Stable_C0:
			return 173;
		case ESIPResourcePlantCollapseBand::Weathered_C1:
			return 431;
		case ESIPResourcePlantCollapseBand::Fractured_C2:
			return 863;
		case ESIPResourcePlantCollapseBand::Unstable_C3:
			return 1297;
		default:
			return 431;
		}
	}

	float CalculateFamilyPenalty(const FSIPResourcePlantCollapseVector& Vector, ESIPResourcePlantPreviewFamily Family)
	{
		float Penalty = 0.f;

		switch (Family)
		{
		case ESIPResourcePlantPreviewFamily::CrownLily:
			// Crown Lily reads best when the lantern core is at least as coherent as the crown.
			if (Vector.PrimarySilhouette >= 2 && Vector.Core == 0)
			{
				Penalty += 0.45f;
			}
			if (Vector.OrbitSet >= 3 && Vector.PrimarySilhouette <= 1)
			{
				Penalty += 0.35f;
			}
			break;
		case ESIPResourcePlantPreviewFamily::AetherVine:
			// Aether Vine can tolerate a wild orbit, but the arc should not lag too far behind it.
			if (Vector.OrbitSet - Vector.PrimarySilhouette >= 3)
			{
				Penalty += 0.55f;
			}
			if (Vector.BodyShell >= 3 && Vector.PrimarySilhouette <= 1)
			{
				Penalty += 0.40f;
			}
			break;
		default:
			break;
		}

		return Penalty;
	}

	float CalculateBandPenalty(const FSIPResourcePlantCollapseVector& Vector, ESIPResourcePlantCollapseBand Band)
	{
		float Penalty = 0.f;
		const int32 MaxLevel = FMath::Max(FMath::Max(Vector.BodyShell, Vector.PrimarySilhouette), FMath::Max(Vector.Core, Vector.OrbitSet));
		const int32 MinLevel = FMath::Min(FMath::Min(Vector.BodyShell, Vector.PrimarySilhouette), FMath::Min(Vector.Core, Vector.OrbitSet));

		switch (Band)
		{
		case ESIPResourcePlantCollapseBand::Stable_C0:
			if (MaxLevel >= 2)
			{
				Penalty += 1.2f;
			}
			break;
		case ESIPResourcePlantCollapseBand::Weathered_C1:
			if (MaxLevel >= 3)
			{
				Penalty += 0.55f;
			}
			break;
		case ESIPResourcePlantCollapseBand::Fractured_C2:
			if (MaxLevel <= 1)
			{
				Penalty += 0.75f;
			}
			break;
		case ESIPResourcePlantCollapseBand::Unstable_C3:
			if (MaxLevel <= 1)
			{
				Penalty += 1.2f;
			}
			if (MinLevel >= 3)
			{
				Penalty += 2.5f;
			}
			break;
		default:
			break;
		}

		return Penalty;
	}

	bool PassesBandEnvelope(const FSIPResourcePlantCollapseMetrics& Metrics, ESIPResourcePlantCollapseBand Band)
	{
		switch (Band)
		{
		case ESIPResourcePlantCollapseBand::Stable_C0:
			return Metrics.Severity <= 0.8f;
		case ESIPResourcePlantCollapseBand::Weathered_C1:
			return Metrics.Severity >= 0.35f && Metrics.Severity <= 1.65f;
		case ESIPResourcePlantCollapseBand::Fractured_C2:
			return Metrics.Severity >= 0.95f && Metrics.Severity <= 2.55f;
		case ESIPResourcePlantCollapseBand::Unstable_C3:
			return Metrics.Severity >= 1.8f;
		default:
			return true;
		}
	}

	float CalculateCandidateEnergy(
		const FSIPResourcePlantCollapseVector& Vector,
		const FSIPResourcePlantCollapseMetrics& Metrics,
		const FCollapseBandProfile& Profile,
		ESIPResourcePlantCollapseBand Band,
		ESIPResourcePlantPreviewFamily Family)
	{
		float Energy = 2.f * Square(Metrics.Severity - Profile.TargetSeverity)
			+ Square(Metrics.Dispersion - Profile.TargetDispersion)
			+ CalculateBandPenalty(Vector, Band)
			+ CalculateFamilyPenalty(Vector, Family);

		if (!Metrics.bPassedHardConstraints)
		{
			Energy += 100.f;
		}

		return Energy;
	}
}

namespace SIPResourcePlantAssembly
{
	FSIPResourcePlantCollapseVector SanitizeCollapseVector(const FSIPResourcePlantCollapseVector& Vector)
	{
		FSIPResourcePlantCollapseVector Sanitized = Vector;
		Sanitized.BodyShell = FMath::Clamp(Sanitized.BodyShell, 0, 3);
		Sanitized.PrimarySilhouette = FMath::Clamp(Sanitized.PrimarySilhouette, 0, 3);
		Sanitized.Core = FMath::Clamp(Sanitized.Core, 0, 3);
		Sanitized.OrbitSet = FMath::Clamp(Sanitized.OrbitSet, 0, 3);
		return Sanitized;
	}

	FSIPResourcePlantCollapseMetrics CalculateCollapseMetrics(const FSIPResourcePlantCollapseVector& Vector)
	{
		const FSIPResourcePlantCollapseVector Sanitized = SanitizeCollapseVector(Vector);

		FSIPResourcePlantCollapseMetrics Metrics;
		Metrics.Severity = BodyShellWeight * Sanitized.BodyShell
			+ PrimarySilhouetteWeight * Sanitized.PrimarySilhouette
			+ CoreWeight * Sanitized.Core
			+ OrbitSetWeight * Sanitized.OrbitSet;

		Metrics.Dispersion = BodyShellWeight * Square(Sanitized.BodyShell - Metrics.Severity)
			+ PrimarySilhouetteWeight * Square(Sanitized.PrimarySilhouette - Metrics.Severity)
			+ CoreWeight * Square(Sanitized.Core - Metrics.Severity)
			+ OrbitSetWeight * Square(Sanitized.OrbitSet - Metrics.Severity);

		Metrics.bPassedHardConstraints = PassesHardConstraints(Sanitized);
		return Metrics;
	}

	bool PassesHardConstraints(const FSIPResourcePlantCollapseVector& Vector)
	{
		const FSIPResourcePlantCollapseVector Sanitized = SanitizeCollapseVector(Vector);

		if (FMath::Abs(Sanitized.PrimarySilhouette - Sanitized.BodyShell) > 2)
		{
			return false;
		}

		if (FMath::Abs(Sanitized.Core - Sanitized.PrimarySilhouette) > 2)
		{
			return false;
		}

		if (Sanitized.Core >= 3 && Sanitized.PrimarySilhouette <= 1)
		{
			return false;
		}

		if (Sanitized.PrimarySilhouette >= 3 && Sanitized.BodyShell <= 0)
		{
			return false;
		}

		return true;
	}

	FSIPResourcePlantCollapseVector SampleCollapseVector(int32 Seed, ESIPResourcePlantCollapseBand Band, ESIPResourcePlantPreviewFamily Family, float TemperatureScale)
	{
		const FCollapseBandProfile Profile = GetBandProfile(Band);
		const float EffectiveTemperature = FMath::Max(0.025f, Profile.Temperature * FMath::Clamp(TemperatureScale, 0.05f, 2.5f));
		const uint32 StreamSeed = static_cast<uint32>(Seed)
			^ (static_cast<uint32>(GetBandSalt(Band)) * 2654435761u)
			^ ((static_cast<uint32>(Family) + 1u) * 2246822519u);
		FRandomStream Random(static_cast<int32>(StreamSeed));

		struct FCandidate
		{
			FSIPResourcePlantCollapseVector Vector;
			float Weight = 0.f;
		};

		TArray<FCandidate> Candidates;
		Candidates.Reserve(256);

		float TotalWeight = 0.f;

		for (int32 BodyShell = 0; BodyShell <= 3; ++BodyShell)
		{
			for (int32 PrimarySilhouette = 0; PrimarySilhouette <= 3; ++PrimarySilhouette)
			{
				for (int32 Core = 0; Core <= 3; ++Core)
				{
					for (int32 OrbitSet = 0; OrbitSet <= 3; ++OrbitSet)
					{
						FSIPResourcePlantCollapseVector CandidateVector;
						CandidateVector.BodyShell = BodyShell;
						CandidateVector.PrimarySilhouette = PrimarySilhouette;
						CandidateVector.Core = Core;
						CandidateVector.OrbitSet = OrbitSet;

						const bool bAllSlotsMaxed = BodyShell == 3 && PrimarySilhouette == 3 && Core == 3 && OrbitSet == 3;
						if (bAllSlotsMaxed)
						{
							continue;
						}

						const FSIPResourcePlantCollapseMetrics Metrics = CalculateCollapseMetrics(CandidateVector);
						if (!Metrics.bPassedHardConstraints)
						{
							continue;
						}
						if (!PassesBandEnvelope(Metrics, Band))
						{
							continue;
						}

						const float Energy = CalculateCandidateEnergy(CandidateVector, Metrics, Profile, Band, Family);
						const float Weight = FMath::Exp(-Energy / EffectiveTemperature);
						if (Weight <= SMALL_NUMBER)
						{
							continue;
						}

						TotalWeight += Weight;
						Candidates.Add({ CandidateVector, Weight });
					}
				}
			}
		}

		if (Candidates.IsEmpty())
		{
			return {};
		}

		const float Pick = Random.FRandRange(0.f, TotalWeight);
		float Accumulator = 0.f;
		for (const FCandidate& Candidate : Candidates)
		{
			Accumulator += Candidate.Weight;
			if (Pick <= Accumulator)
			{
				return Candidate.Vector;
			}
		}

		return Candidates.Last().Vector;
	}

	float LevelAlpha(int32 Level)
	{
		return static_cast<float>(FMath::Clamp(Level, 0, 3)) / 3.f;
	}

	const FSIPResourcePlantSlotAssemblyHint* FindSlotHint(const FSIPResourcePlantAssemblyHint& Hint, ESIPResourcePlantSlot Slot)
	{
		return Hint.Slots.FindByPredicate([Slot](const FSIPResourcePlantSlotAssemblyHint& SlotHint)
		{
			return SlotHint.Slot == Slot;
		});
	}

	FVector ConvertSourceOffsetToUnreal(const FVector& SourceOffset, ESIPResourcePlantAssemblyAxisMapping AxisMapping, float SourceToUnrealScale)
	{
		FVector MappedOffset = SourceOffset;

		switch (AxisMapping)
		{
		case ESIPResourcePlantAssemblyAxisMapping::SourceYUpToUnrealZUp:
			MappedOffset = FVector(SourceOffset.X, -SourceOffset.Z, SourceOffset.Y);
			break;
		case ESIPResourcePlantAssemblyAxisMapping::Identity:
		default:
			break;
		}

		return MappedOffset * FMath::Max(SourceToUnrealScale, 0.001f);
	}

	FTransform ResolveSlotTransform(const FSIPResourcePlantAssemblyHint& Hint, ESIPResourcePlantSlot Slot)
	{
		const FSIPResourcePlantSlotAssemblyHint* SlotHint = FindSlotHint(Hint, Slot);
		if (!SlotHint)
		{
			return FTransform::Identity;
		}

		const FVector SourceOffset = SlotHint->SourcePlacementCenter - Hint.SourceAnchor;
		return FTransform(
			SlotHint->LocalRotation,
			ConvertSourceOffsetToUnreal(SourceOffset, Hint.AxisMapping, Hint.SourceToUnrealScale),
			SlotHint->LocalScale);
	}
}
