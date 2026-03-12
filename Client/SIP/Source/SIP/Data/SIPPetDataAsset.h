#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SIPPetDataAsset.generated.h"

class USIPAbilitySet;

UCLASS(BlueprintType, Const)
class SIP_API USIPPetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 宠物的基础模型
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> PetMesh;

	// 宠物的初始技能包
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<USIPAbilitySet> DefaultAbilitySet;

	// 宠物的初始数值
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
	float BaseHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes")
	float BaseWorkSpeed = 1.0f;
};
