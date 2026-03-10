// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPElementalZoneActor.h"

#include "AbilitySystemComponent.h"
#include "World/SIPElementReactionSubsystem.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PCGComponent.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

ASIPElementalZoneActor::ASIPElementalZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	ZoneBounds->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = ZoneBounds;

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}

void ASIPElementalZoneActor::BeginPlay()
{
    Super::BeginPlay();

    // 植物区域在进入游戏时自动生成 PCG 植被
    // 延迟 0.1 秒执行：PCGWorldActor / GraphExecutor 在 BeginPlay 同帧内可能尚未完成初始化
    if (bAutoGenerateVegetation && ZoneElementTag == SIPGameplayTags::Element_Plant)
    {
        FTimerHandle DummyHandle;
        GetWorldTimerManager().SetTimer(DummyHandle, [this]()
        {
            GenerateVegetation(VegetationSeed);
        }, 0.1f, false);
    }
}

void ASIPElementalZoneActor::ReceiveElementHit(const FGameplayTag& IncomingElement, const FVector& ImpactLocation)
{
	// 已经发生过反应则忽略（防止重复触发）
	if (ZoneStateTag.IsValid())
	{
		UE_LOG(LogSIP, Verbose, TEXT("%s already in state [%s], ignoring new hit."),
			*GetName(), *ZoneStateTag.ToString());
		return;
	}

	if (!ZoneElementTag.IsValid())
	{
		UE_LOG(LogSIP, Warning, TEXT("%s has no ZoneElementTag configured!"), *GetName());
		return;
	}

	USIPElementReactionSubsystem* Subsystem = GetWorld()->GetSubsystem<USIPElementReactionSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	// 查询反应
	const FGameplayTag ReactionTag = Subsystem->QueryReaction(ZoneElementTag, IncomingElement);
	if (!ReactionTag.IsValid())
	{
		return;
	}

	// 广播反应事件（UI、音效等系统可监听）
	Subsystem->ProcessElementHit(ZoneElementTag, IncomingElement, ImpactLocation, this);

	// 执行本地可见变化
	ApplyReaction(ReactionTag, ImpactLocation);
}

void ASIPElementalZoneActor::ApplyReaction(const FGameplayTag& ReactionTag, const FVector& ImpactLocation)
{
	// 记录状态，防止重复触发
	if (ReactionTag == SIPGameplayTags::Reaction_Burn)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Burning;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Melt)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Frozen; // 原本是冰，融化后标记为已处理
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Freeze)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Frozen;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Electrify)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Electrified;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Bloom)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Bloomed;
	}

	// 感电反应：对区域内敌人施加眩晕 Tag
	if (ReactionTag == SIPGameplayTags::Reaction_Electrify)
	{
		TArray<AActor*> OverlappingActors;
		GetOverlappingActors(OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
			{
				// 添加 AbilityInputBlocked Tag，持续 3 秒后由 GE 自动移除
				// 此处直接 AddLooseGameplayTag 作为 POC 实现
				// 生产环境应通过 GE_Electrify 来管理时长
				ASC->AddLooseGameplayTag(SIPGameplayTags::TAG_Gameplay_AbilityInputBlocked);
				UE_LOG(LogSIP, Log, TEXT("Electrify stunned: %s"), *Actor->GetName());
			}
		}
	}

	// 隐藏默认状态 Actor（树木、冰柱等）
	HideDefaultActors();

	// 清理 PCG 生成的内容
	ClearPCGContent();

	// 生成反应后的 Actor（冰平台、焦土模型等）
	SpawnReactionActor(ReactionTag, ImpactLocation);

	// 播放落点 VFX
	PlayReactionVFX(ReactionTag, ImpactLocation);

	// 绽放反应：植被清除后以新种子重新生成更茂盛的植被
	if (ReactionTag == SIPGameplayTags::Reaction_Bloom)
	{
		// 用基础种子 + 随机偏移保证每次 Bloom 外观不同
		const int32 BloomSeed = VegetationSeed + FMath::RandRange(1000, 9999);
		GenerateVegetation(BloomSeed);
		UE_LOG(LogSIP, Log, TEXT("%s Bloom: vegetation regenerated with seed=%d."), *GetName(), BloomSeed);
	}

	// 通知 Blueprint 扩展
	K2_OnReactionApplied(ReactionTag, ImpactLocation);

	UE_LOG(LogSIP, Log, TEXT("%s applied reaction [%s] at %s"),
		*GetName(), *ReactionTag.ToString(), *ImpactLocation.ToString());
}

void ASIPElementalZoneActor::HideDefaultActors()
{
	for (AActor* Actor : LinkedDefaultActors)
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
		}
	}
}

void ASIPElementalZoneActor::SpawnReactionActor(const FGameplayTag& ReactionTag, const FVector& Location)
{
	const TSubclassOf<AActor>* SpawnClass = ReactionSpawnClassMap.Find(ReactionTag);
	if (!SpawnClass || !*SpawnClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	GetWorld()->SpawnActor<AActor>(*SpawnClass, Location, FRotator::ZeroRotator, SpawnParams);
}

void ASIPElementalZoneActor::PlayReactionVFX(const FGameplayTag& ReactionTag, const FVector& Location)
{
	auto* VFXPtr = ReactionVFXMap.Find(ReactionTag);
	if (!VFXPtr || !*VFXPtr)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		*VFXPtr,
		Location,
		FRotator::ZeroRotator,
		FVector::OneVector,
		true,  // auto-destroy
		true
	);
}

void ASIPElementalZoneActor::GenerateVegetation(int32 Seed)
{
	if (!PCGComponent)
	{
		return;
	}

	// 必须在 PCGComponent 的 Graph 属性中配置好资产，否则无法生成
	// 操作路径：Blueprint 编辑器 Components 面板 → 选中 PCGComponent → Details 面板 → Graph
	if (!PCGComponent->GetGraph())
	{
		UE_LOG(LogSIP, Warning,
			TEXT("%s GenerateVegetation: PCGComponent has no Graph assigned! "
			     "Please set it in Blueprint editor: Components panel -> PCGComponent -> Details -> Graph."),
			*GetName());
		return;
	}

	const int32 UsedSeed = (Seed != 0) ? Seed : VegetationSeed;
	PCGComponent->Seed = UsedSeed;
	PCGComponent->GenerateLocal(false);

	UE_LOG(LogSIP, Log, TEXT("%s vegetation generated (Seed=%d)."), *GetName(), UsedSeed);
}

void ASIPElementalZoneActor::ClearPCGContent()
{
	if (!PCGComponent)
	{
		return;
	}

	// 清理 PCG 在这个 Actor 上生成的所有内容（树木、岩石等）
	PCGComponent->CleanupLocal(true);
	UE_LOG(LogSIP, Log, TEXT("%s PCG content cleared."), *GetName());
}
