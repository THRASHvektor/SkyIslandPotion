#include "SIPPlayerController.h"
#include "Character/SIPCharacter.h"
#include "Ability/SIPAbilitySystemComponent.h"

// 当前没有额外构造逻辑，保留显式构造函数方便后续扩展控制器侧初始化。
ASIPPlayerController::ASIPPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// 在 Enhanced Input 更新完本帧动作后，统一驱动 ASC 处理能力输入。
void ASIPPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
    Super::PostProcessInput(DeltaTime, bGamePaused);

    if (ASIPCharacter* character = Cast<ASIPCharacter>(GetPawn()))
    {
        if (USIPAbilitySystemComponent* ASC = character->GetSIPAbilitySystemComponent())
        {
            ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
        }
    }
}
