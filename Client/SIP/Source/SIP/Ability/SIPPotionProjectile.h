// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * ASIPPotionProjectile 是投掷药水的弹丸 Actor
 *
 * 行为流程：
 * 1. GA_ThrowPotion 生成此 Actor，设置 ElementTag 和初速度
 * 2. 弹丸沿抛物线飞行（ProjectileMovement 负责）
 * 3. 命中任何物体后触发 OnProjectileHit：
 *    a. 球形检测范围内的 SIPCharacter → ApplyCombatDamage（直接伤害）
 *    b. 球形检测范围内的 SIPElementalZoneActor → ReceiveElementHit（触发元素反应）
 * 4. 播放落点 VFX，销毁自身
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SIPPotionProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(Blueprintable)
class SIP_API ASIPPotionProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASIPPotionProjectile();

	/** 由 GA_ThrowPotion 在 Spawn 后立即设置 */
	UPROPERTY(BlueprintReadWrite, Category = "SIP|Projectile")
	FGameplayTag ElementTag;

	/** 碰撞胶囊（根组件） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** 视觉网格（瓶身） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Projectile")
	TObjectPtr<UStaticMeshComponent> PotionMesh;

	/** 飞行拖尾特效组件（飞行中持续播放） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Projectile")
	TObjectPtr<UNiagaraComponent> TrailFX;

	/** 抛体移动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 落点 AOE 半径（同时作用于战斗伤害和元素反应检测） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Projectile")
	float ImpactRadius = 200.f;

	/** 对命中角色的直接伤害 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Projectile")
	float ImpactDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Projectile", meta = (ClampMin = "0.0"))
	float SurfaceDamage = 1.0f;

	/** 落点爆炸特效（每种元素可设置不同 VFX）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Projectile")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	/** 命中后的核心处理：AOE 伤害 + 元素区域通知 + VFX */
	void HandleImpact(const FVector& ImpactLocation);
};
