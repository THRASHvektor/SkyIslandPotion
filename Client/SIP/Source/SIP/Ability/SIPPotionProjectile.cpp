// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/SIPPotionProjectile.h"
#include "World/SIPElementalZoneActor.h"
#include "World/SIPElementImpactReceiver.h"
#include "World/SIPElementImpactTypes.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "Character/SIPCharacter.h"
#include "Combat/SIPCombatStatics.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "SIPLogCategory.h"

ASIPPotionProjectile::ASIPPotionProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 根碰撞球
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(12.f);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);   // 撞地面/墙壁
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);  // 撞 ZoneActor
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);         // 忽略所有 Pawn（防止撞到投掷者）
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionSphere;

	// 瓶身网格
	PotionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PotionMesh"));
	PotionMesh->SetupAttachment(RootComponent);
	PotionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 拖尾 VFX（飞行中播放）
	TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailFX"));
	TrailFX->SetupAttachment(RootComponent);

	// 抛体移动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->ProjectileGravityScale = 0.6f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	// 弹丸落地/命中物体后自动销毁（1.5 秒保底）
	InitialLifeSpan = 5.0f;
}

void ASIPPotionProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentHit.AddDynamic(this, &ASIPPotionProjectile::OnProjectileHit);
}

void ASIPPotionProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	HandleImpact(Hit.ImpactPoint);
}

void ASIPPotionProjectile::HandleImpact(const FVector& ImpactLocation)
{
	// ── 1. 球形检测：仅用于对角色造成 AOE 伤害 ─────────────────────
	//     元素反应【不再】依赖此结果，避免 VisualActor / 其它无关碰撞
	//     被误认为反应目标。
	TArray<FHitResult> HitResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn)
	};

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		this,
		ImpactLocation,
		ImpactLocation + FVector::UpVector * 1.f, // 原地球形
		ImpactRadius,
		ObjectTypes,
		false,
		TArray<AActor*>{ GetInstigator() },
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	TSet<AActor*> DamagedActors; // 防止同一 Actor 被打两次
	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}
		DamagedActors.Add(HitActor);

		if (ASIPCharacter* TargetChar = Cast<ASIPCharacter>(HitActor))
		{
			// 重构：区域伤害统一走 GAS GameplayEffect 流程（项目默认伤害 GE）。
			USIPCombatStatics::ApplyDamageToTarget(TargetChar, ImpactDamage, GetInstigator(), this, nullptr);
		}
	}

	// ── 2. 元素反应：严格由 ZoneActor 的 ZoneBounds 决定 ─────────────
	//     不依赖 SphereTrace / VisualActor 的碰撞——遍历世界中所有
	//     Zone，用其组件包围盒与 AOE 球做几何相交测试。
	if (ElementTag.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			const float SphereRadiusSq = ImpactRadius * ImpactRadius;
			TSet<AActor*> ProcessedZones;

			auto TryTriggerZone = [&](AActor* ZoneActor)
			{
				if (!ZoneActor || ProcessedZones.Contains(ZoneActor))
				{
					return;
				}

				// 只使用 Zone 自身注册的碰撞组件包围盒（ZoneBounds），
				// 忽略 VisualActor / 附加子 Actor 的碰撞体积。
				const FBox ZoneBox = ZoneActor->GetComponentsBoundingBox(false, false);
				if (!ZoneBox.IsValid)
				{
					return;
				}

				const FVector ClosestPoint = ZoneBox.GetClosestPointTo(ImpactLocation);
				if (FVector::DistSquared(ClosestPoint, ImpactLocation) > SphereRadiusSq)
				{
					return;
				}

				ProcessedZones.Add(ZoneActor);

				// 优先走接口（新 Zone 系统 ASIPElementReactiveZoneBase）
				if (ZoneActor->GetClass()->ImplementsInterface(USIPElementImpactReceiver::StaticClass()))
				{
					FSIPElementImpactContext ImpactContext;
					ImpactContext.IncomingElement = ElementTag;
					ImpactContext.SurfaceDamage = SurfaceDamage;
					ImpactContext.ImpactLocation = ImpactLocation;
					ImpactContext.SourceActor = this;
					ImpactContext.InstigatorActor = GetInstigator();

					ISIPElementImpactReceiver::Execute_ReceiveElementImpact(ZoneActor, ImpactContext);
				}
				else if (ASIPElementalZoneActor* LegacyZone = Cast<ASIPElementalZoneActor>(ZoneActor))
				{
					LegacyZone->ReceiveElementHit(ElementTag, ImpactLocation);
				}
			};

			// 新 Zone 系统：全部继承自 ASIPElementReactiveZoneBase
			for (TActorIterator<ASIPElementReactiveZoneBase> It(World); It; ++It)
			{
				TryTriggerZone(*It);
			}

			// 老 Zone 系统兼容
			for (TActorIterator<ASIPElementalZoneActor> It(World); It; ++It)
			{
				TryTriggerZone(*It);
			}
		}
	}

	// ── 3. 播放落点 VFX ────────────────────────────────────────────
	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactVFX,
			ImpactLocation,
			FRotator::ZeroRotator,
			FVector::OneVector,
			true,
			true
		);
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("PotionProjectile[%s] impacted at %s. Damaged %d actors."),
		*ElementTag.ToString(), *ImpactLocation.ToString(), DamagedActors.Num());

	Destroy();
}
