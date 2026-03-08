// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * ASIPHeroCharacter 是玩家控制的主角角色类
 * 继承自 ASIPCharacter，是玩家在游戏中实际控制的对象
 * 
 * 主要功能：
 * 1. 管理摄像机（SpringArm + FollowCamera）
 * 2. 处理增强输入系统（Enhanced Input）
 * 3. 绑定输入到技能系统
 * 4. 初始化并授予角色技能
 * 
 * 设计思路：
 * - 使用 Enhanced Input 系统，支持更灵活的输入配置
 * - 通过 InputConfig 数据资产配置输入映射
 * - 继承自 ASIPCharacter，复用 GAS 能力系统
 */

#pragma once

#include "CoreMinimal.h"
#include "SIPCharacter.h"
#include "Input/SIPInputConfig.h"
#include "SIPHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UGameplayEffect;
struct FInputActionValue;
struct FActiveGameplayEffectHandle;


/**
 * Z 说明：
 * TODO 注释：代码优化方向
 * 1. 目前先在角色类中直接绑定输入，后续最好通过输入组件的方式来实现
 * 2. 最好把玩家的操控独立成一个组件，方便随时切换操作对象（宠物、坐骑）
 */

/**
 * Z 说明：
 * ASIPHeroCharacter 是玩家所操控的英雄类
 * 
 * 继承层次：
 * AActor → APawn → ACharacter → ASIPCharacter → ASIPHeroCharacter
 * 
 * 使用场景：
 * - 玩家控制的角色
 * - 需要摄像机跟随
 * - 需要技能系统
 */
UCLASS(config=Game)
class ASIPHeroCharacter : public ASIPCharacter
{
	GENERATED_BODY()

public:
	/**
	 * Z 说明：构造函数
	 * 初始化组件和角色参数
	 */
	ASIPHeroCharacter(const FObjectInitializer& ObjectInitializer);

	/**
	 * Z 说明：摄像机臂组件
	 * 用于控制第三人称视角的距离和旋转
	 * 
	 * 功能：
	 * - 拉近/拉远摄像机
	 * - 碰撞时自动收缩
	 * - 跟随角色旋转
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/**
	 * Z 说明：跟随摄像机
	 * 绑定到 CameraBoom，自动跟随角色
	 * 
	 * 特点：
	 * - 不随控制器旋转，独立于手臂旋转
	 * - 提供稳定的第三人称视角
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/**
	 * Z 说明：输入映射上下文
	 * 来自 Enhanced Input 系统
	 * 定义了按键到 InputAction 的映射
	 * 
	 * 使用方式：
	 * 在 Blueprint 中配置此属性，引用项目中的 IMC 资源
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;

	/**
	 * Z 说明：输入配置数据资产
	 * 定义了 InputAction 到 GameplayTag 的映射
	 * 
	 * 作用：
	 * - NativeInputActions: 手动绑定的输入（移动、视角）
	 * - AbilityInputActions: 自动绑定到技能的输入（攻击、跳跃）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input")
	TObjectPtr<USIPInputConfig> InputConfig;

	/**
	 * Z 说明：获取摄像机臂组件
	 */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	/**
	 * Z 说明：获取跟随摄像机
	 */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:


	/**
	 * Z 说明：技能输入按下回调
	 * 当玩家按下技能按键时调用
	 * 将 InputTag 传递给 ASC 进行处理
	 */
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);

	/**
	 * Z 说明：技能输入释放回调
	 * 当玩家释放技能按键时调用
	 * 将 InputTag 传递给 ASC 进行处理
	 */
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	/**
	 * Z 说明：移动输入处理
	 * 处理 WASD/手柄摇杆输入
	 * 将输入转换为角色移动方向
	 */
	void Input_Move(const FInputActionValue& Value);

	/**
	 * Z 说明：视角输入处理
	 * 处理鼠标移动/手柄右摇杆
	 * 控制摄像机旋转
	 */
	void Input_Look(const FInputActionValue& Value);

	/**
	 * Z 说明：设置输入组件
	 * UE 生命周期函数
	 * 在此绑定增强输入系统
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * Z 说明：初始化组件
	 * 在此授予角色技能
	 */
	virtual void PostInitializeComponents() override;
	
	/**
	 * Z 说明：开始播放
	 * 可以在此添加角色初始化逻辑
	 */
	virtual void BeginPlay();

	
};
