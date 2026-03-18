// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPIslandActor.h"
#include "World/SIPIslandGeneratorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "SIPLogCategory.h"
#include "TimerManager.h"

ASIPIslandActor::ASIPIslandActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// ── 根组件：PCG 采样边界盒 ──────────────────────────────────────
	// 作为根组件，IslandMesh 附着在此之下
	// Extent 默认 1000x1000x400（cm），对应约 20m x 20m x 8m 的小型悬浮岛
	// 在 BP 子类中按美术网格的实际尺寸调整 BoxExtent
	IslandBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("IslandBounds"));
	IslandBounds->SetBoxExtent(FVector(1000.f, 1000.f, 400.f));
	// PCG bounds filtering may rely on queryable primitive data; keep bounds query-only.
	IslandBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	IslandBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	IslandBounds->SetHiddenInGame(true);
	IslandBounds->ComponentTags.Add(TEXT("PCG_Bounds"));
	RootComponent = IslandBounds;

	// ── 岛屿视觉网格 ────────────────────────────────────────────────
	// BlockAll：玩家/Enemy 均可站立
	// 无默认网格资产，在 BP 子类中指定（SM_RockIsland_Forest 等）
	IslandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IslandMesh"));
	IslandMesh->SetupAttachment(RootComponent);
	IslandMesh->SetCollisionProfileName(TEXT("BlockAll"));
	IslandMesh->ComponentTags.Add(TEXT("PCG_Surface"));

	// ── PCG 生成组件 ─────────────────────────────────────────────────
	// bGenerateOnBeginPlay = false：不在 BeginPlay 自动生成
	// 由 InitializeIsland() 在 Biome/Seed 注入后手动触发，保证顺序正确
	IslandGenerator = CreateDefaultSubobject<USIPIslandGeneratorComponent>(TEXT("IslandGenerator"));
	IslandGenerator->bGenerateOnBeginPlay = false;
}

void ASIPIslandActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASIPIslandActor::InitializeIsland(const FGameplayTag& InBiome, int32 InIndex, int32 WorldSeed)
{
	// ── 1. 注入数据 ──────────────────────────────────────────────────
	BiomeType  = InBiome;
	IslandIndex = InIndex;
	bIsSeamCandidate = false;
	NearbyIslandCount = 0;

	IslandGenerator->BiomeType   = InBiome;
	// 子 Seed 公式：WorldSeed + Index*1000，确保每个岛唯一且可复现
	IslandGenerator->DefaultSeed = WorldSeed + InIndex * 1000;

	// ── 1.5 Mesh 变体随机选取（Seed 确定性）──────────────────────
	// MeshVariantsPerBiome 在 BP 子类中配置，若为空则保持 BP 默认 Mesh 不变
	if (const FSIPMeshVariantList* Variants = MeshVariantsPerBiome.Find(InBiome))
	{
		if (Variants->Meshes.Num() > 0)
		{
			FRandomStream MeshRand(IslandGenerator->DefaultSeed);
			const int32 MeshIdx = MeshRand.RandRange(0, Variants->Meshes.Num() - 1);
			if (UStaticMesh* Chosen = Variants->Meshes[MeshIdx])
			{
				IslandMesh->SetStaticMesh(Chosen);
				UE_LOG(LogSIP, Log, TEXT("IslandActor[%d] Mesh variant [%d/%d] selected for Biome=[%s]"),
					InIndex, MeshIdx, Variants->Meshes.Num() - 1, *InBiome.ToString());
			}
		}
	}
	// ── 2. 延迟触发 PCG 生成 ────────────────────────────────────────
	FTimerHandle GenerateHandle;
	GetWorldTimerManager().SetTimer(GenerateHandle, [this]()
	{
		if (IsValid(this))
		{
			IslandGenerator->GenerateIsland(IslandGenerator->DefaultSeed);
		}
	}, 0.15f, false);

	// ── 3. 通知 Blueprint 子类做视觉处理（材质切换、后处理颜色等）───
	K2_OnBiomeInitialized(InBiome);

	UE_LOG(LogSIP, Log, TEXT("IslandActor[%d] initialized. Biome=[%s] Seed=[%d]"),
		InIndex, *InBiome.ToString(), IslandGenerator->DefaultSeed);
	UE_LOG(LogSIP, Verbose, TEXT("IslandActor[%d] tags: Bounds=%d Surface=%d"),
		InIndex,
		IslandBounds->ComponentTags.Contains(TEXT("PCG_Bounds")) ? 1 : 0,
		IslandMesh->ComponentTags.Contains(TEXT("PCG_Surface")) ? 1 : 0);
}

float ASIPIslandActor::GetPlacementRadius2D() const
{
	if (!IslandBounds)
	{
		return 0.f;
	}

	const FVector Extent = IslandBounds->GetScaledBoxExtent();
	return FMath::Max(Extent.X, Extent.Y);
}

float ASIPIslandActor::GetPlacementHalfHeight() const
{
	return IslandBounds ? IslandBounds->GetScaledBoxExtent().Z : 0.f;
}

void ASIPIslandActor::RegisterNearbyIsland(ASIPIslandActor* OtherIsland)
{
	if (!IsValid(OtherIsland) || OtherIsland == this)
	{
		return;
	}

	bIsSeamCandidate = true;
	++NearbyIslandCount;
}
