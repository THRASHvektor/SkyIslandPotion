// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/SIPPotionProjectile.h"
#include "World/SIPElementalZoneActor.h"
#include "World/SIPElementImpactReceiver.h"
#include "World/SIPElementImpactTypes.h"
#include "Character/SIPCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/KismetSystemLibrary.h"
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
	// ── 1. 球形检测所有命中对象 ──────────────────────────────────────
	TArray<FHitResult> HitResults;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic)
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

	// ── 2. 对角色造成伤害，对 ZoneActor 触发元素反应 ────────────────
	TSet<AActor*> ProcessedActors; // 防止同一 Actor 处理两次

	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || ProcessedActors.Contains(HitActor))
		{
			continue;
		}
		ProcessedActors.Add(HitActor);

		// 角色伤害
		if (ASIPCharacter* TargetChar = Cast<ASIPCharacter>(HitActor))
		{
			TargetChar->ApplyCombatDamage(ImpactDamage, GetInstigator());
		}

		// 元素区域反应
		if (!ElementTag.IsValid())
		{
			continue;
		}

		if (HitActor->GetClass()->ImplementsInterface(USIPElementImpactReceiver::StaticClass()))
		{
			FSIPElementImpactContext ImpactContext;
			ImpactContext.IncomingElement = ElementTag;
			ImpactContext.SurfaceDamage = SurfaceDamage;
			ImpactContext.ImpactLocation = ImpactLocation;
			ImpactContext.SourceActor = this;
			ImpactContext.InstigatorActor = GetInstigator();

			ISIPElementImpactReceiver::Execute_ReceiveElementImpact(HitActor, ImpactContext);
		}
		else if (ASIPElementalZoneActor* Zone = Cast<ASIPElementalZoneActor>(HitActor))
		{
			Zone->ReceiveElementHit(ElementTag, ImpactLocation);
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

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("PotionProjectile[%s] impacted at %s. Hit %d actors."),
		*ElementTag.ToString(), *ImpactLocation.ToString(), ProcessedActors.Num());

	Destroy();
}
