#include "SIPPetAttributeSet.h"
#include "GameplayEffectExtension.h"

USIPPetAttributeSet::USIPPetAttributeSet()
{
	// 这里可以写初始默认值，但我们已经在 DataAsset 里配了，所以留空即可
}

void USIPPetAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 如果策划/技能试图改变血量，我们在这里拦截，保证它永远在 0 到 最大血量 之间
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxHealth());
	}
}
