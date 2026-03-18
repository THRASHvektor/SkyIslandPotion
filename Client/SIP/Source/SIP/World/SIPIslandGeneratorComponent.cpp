// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPIslandGeneratorComponent.h"
#include "PCGComponent.h"
#include "SIPLogCategory.h"

USIPIslandGeneratorComponent::USIPIslandGeneratorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USIPIslandGeneratorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateIsland(DefaultSeed);
	}
}

void USIPIslandGeneratorComponent::GenerateIsland(int32 Seed)
{
	UPCGComponent* PCGComp = GetOrCreatePCGComponent();
	if (!PCGComp)
	{
		UE_LOG(LogSIP, Warning, TEXT("[%s] GenerateIsland failed: no PCGComponent on owner."), *GetName());
		return;
	}

	// 根据 Biome 切换 PCG Graph（如果配置了的话）
	if (BiomeType.IsValid())
	{
		auto* GraphPtr = PCGGraphPerBiome.Find(BiomeType);
		if (GraphPtr && *GraphPtr)
		{
			PCGComp->SetGraph(*GraphPtr);
			UE_LOG(LogSIP, Log, TEXT("[%s] PCG Graph set for biome [%s]"), *GetName(), *BiomeType.ToString());
		}
		else
		{
			UE_LOG(LogSIP, Warning, TEXT("[%s] No PCG Graph configured for biome [%s], using default."),
				*GetName(), *BiomeType.ToString());
		}
	}

	// 设置随机种子（决定具体生成结果，同种子同结果）
	const int32 UsedSeed = (Seed != 0) ? Seed : DefaultSeed;
	PCGComp->Seed = UsedSeed;

	if (!PCGComp->GetGraph())
	{
		UE_LOG(LogSIP, Warning,
			TEXT("[%s] GenerateIsland aborted: PCGComponent has no graph. Biome=[%s], Owner=[%s]"),
			*GetName(),
			*BiomeType.ToString(),
			*GetNameSafe(GetOwner()));
		return;
	}

	// 触发生成（异步，UE5 PCG 框架内部处理多线程）
	PCGComp->GenerateLocal(false);

	UE_LOG(LogSIP, Log, TEXT("[%s] Island generation started. Biome=[%s] Seed=[%d]"),
		*GetOwner()->GetName(),
		*BiomeType.ToString(),
		UsedSeed);
}

void USIPIslandGeneratorComponent::ClearIsland()
{
	UPCGComponent* PCGComp = GetOwner()->FindComponentByClass<UPCGComponent>();
	if (!PCGComp)
	{
		return;
	}

	PCGComp->CleanupLocal(true);
	UE_LOG(LogSIP, Log, TEXT("[%s] Island cleared."), *GetOwner()->GetName());
}

UPCGComponent* USIPIslandGeneratorComponent::GetOrCreatePCGComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UPCGComponent* PCGComp = Owner->FindComponentByClass<UPCGComponent>();
	if (!PCGComp)
	{
		// 运行时动态添加 PCGComponent（若 Owner 上还没有）
		PCGComp = NewObject<UPCGComponent>(Owner, TEXT("PCGComponent"));
		PCGComp->RegisterComponent();
		Owner->AddInstanceComponent(PCGComp);
		UE_LOG(LogSIP, Log, TEXT("[%s] PCGComponent created dynamically. Registered=%d"), *Owner->GetName(), PCGComp->IsRegistered() ? 1 : 0);
	}

	return PCGComp;
}
