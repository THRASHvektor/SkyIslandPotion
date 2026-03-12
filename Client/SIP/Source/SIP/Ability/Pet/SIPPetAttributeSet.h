#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SIPPetAttributeSet.generated.h"


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class SIP_API USIPPetAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	USIPPetAttributeSet();

	// 当前血量
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USIPPetAttributeSet, Health)

	// 最大血量
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USIPPetAttributeSet, MaxHealth)

	// 工作速度 (用于宠物干活、产出的效率)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData WorkSpeed;
	ATTRIBUTE_ACCESSORS(USIPPetAttributeSet, WorkSpeed)

	// 拦截器
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
