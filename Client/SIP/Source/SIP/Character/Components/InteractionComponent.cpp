#include "InteractionComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "CollisionChannels.h"

UInteractionComponent::UInteractionComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.TickInterval = 0.25f;
}

void UInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(true);
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    SetComponentTickEnabled(false);
    if(CurrentFocusedInteractable)
    {
        CurrentFocusedInteractable->OnEndFocus();
    }
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    SearchForFocusInteractable();
    if (bShowSearchDebug)
    {
        DrawSearchDebug();
    }
}

void UInteractionComponent::InteractWithActor(AActor* TargetActor, UAbilitySystemComponent* OwnerASC)
{
    if(CurrentFocusedInteractable)
    {
        CurrentFocusedInteractable->Interact(OwnerASC);
    }
}

void UInteractionComponent::SearchForFocusInteractable()
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        // 使用球形碰撞检测来搜索周围的可交互物体
        FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractableSearchRadius);

        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Owner);

        FCollisionObjectQueryParams ObjectQueryParams;
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Interactable);
        
        TArray<FOverlapResult> OutOverlaps;

        // 进行碰撞检测
        bool bHit = GetWorld()->OverlapMultiByObjectType(
            OutOverlaps,
            Owner->GetActorLocation(),
            FQuat::Identity,
            ObjectQueryParams,
            Sphere,
            Params
        );

        // GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, bHit?FString::Printf(TEXT("Hit")):FString::Printf(TEXT("No hit")));

        if(bHit)
        {
            AActor* BestFocusedActor = nullptr;
            float BestScore = -1.0f;

            for (const FOverlapResult& Result : OutOverlaps)
            {
                // 对所有范围内的结果进行评分，选择最佳
                AActor* OverlappedActor = Result.GetActor();
                // 只考虑实现了 IInteractable 接口的 Actor
                if (OverlappedActor && OverlappedActor->Implements<UInteractable>())
                {
                    // 计算评分
                    float Score = CalculateSearchScore(OverlappedActor);
                    if (Score > BestScore)
                    {
                        BestScore = Score;
                        BestFocusedActor = OverlappedActor;
                    }
                }

                if(BestFocusedActor)
                {
                    if (CurrentFocusedInteractable.GetObject() != BestFocusedActor)
                    {
                        // 如果最佳候选与当前不同，更新焦点
                        if (CurrentFocusedInteractable)
                        {
                            CurrentFocusedInteractable->OnEndFocus();
                        }

                        CurrentFocusedInteractable = TScriptInterface<IInteractable>(BestFocusedActor);
                        CurrentFocusedInteractable->OnBeginFocus();
                    }
                }
            }
        }

        else
        {
            // 没有任何可交互物体，清除当前可交互物体
            if (CurrentFocusedInteractable)
            {
                CurrentFocusedInteractable->OnEndFocus();
                CurrentFocusedInteractable = nullptr;
            }
        }
    }
}

float UInteractionComponent::CalculateSearchScore(AActor* TargetActor) const
{
    AActor* Owner = GetOwner();
    if (!Owner || !TargetActor)
    {
        return -1.0f;
    }

    FVector OwnerViewLoc = GetViewLocation();
    FVector OwnerViewDir = GetViewDirection();
    FVector TargetLoc = TargetActor->GetActorLocation();

    // 观察位置到目标的方向向量
    FVector ToTarget = (TargetLoc - OwnerViewLoc).GetSafeNormal();

    // 1. 计算角度分：目标越靠近玩家观察方向，分越大，范围 [0, 1]
    float Dot = FVector::DotProduct(OwnerViewDir, ToTarget);
    float AngleScore = FMath::Max(0.f, Dot);
    // 2. 计算距离分：离玩家越近，分越大，范围 [0, 1]
    float Distance = FVector::Dist(OwnerViewLoc, TargetLoc);
    float DistanceScore = 1.f - FMath::Clamp(Distance / InteractableSearchRadius, 0.f, 1.f);

    // 根据权重计算总分，范围 [0, 1]
    return 0.7f * AngleScore + 0.3f * DistanceScore;
}

FVector UInteractionComponent::GetViewLocation() const
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        // 尝试获取玩家控制器的视点位置
        if (APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController()))
        {
            FVector ViewLoc;
            FRotator ViewRot;
            PC->GetPlayerViewPoint(ViewLoc, ViewRot);
            return ViewLoc;
        }
        // 如果不是玩家，使用角色位置
        return Owner->GetActorLocation();
    }
    return FVector::ZeroVector;
}

FVector UInteractionComponent::GetViewDirection() const
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        // 尝试获取玩家控制器的视点方向
        if (APlayerController* PC = Cast<APlayerController>(Owner->GetInstigatorController()))
        {
            FVector ViewLoc;
            FRotator ViewRot;
            PC->GetPlayerViewPoint(ViewLoc, ViewRot);
            return ViewRot.Vector();
        }
        // 如果不是玩家，使用角色前向量
        return Owner->GetActorForwardVector();
    }
    return FVector::ForwardVector;
}

void UInteractionComponent::DrawSearchDebug() const
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        DrawDebugSphere(GetWorld(), Owner->GetActorLocation(), InteractableSearchRadius, 16, FColor::Green, false, 0.25f);
    }
}