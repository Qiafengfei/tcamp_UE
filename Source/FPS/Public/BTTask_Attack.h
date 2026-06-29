// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Attack.generated.h"

/**
 * 行为树攻击任务 - AI 对目标执行攻击
 * 需要黑板键: TargetActor (Object)
 */
UCLASS()
class FPS_API UBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

	/** 黑板键：目标 Actor */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** 攻击伤害 */
	UPROPERTY(EditAnywhere, Category = "AI")
	float Damage;

	/** 攻击范围 */
	UPROPERTY(EditAnywhere, Category = "AI")
	float RequiredRange;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
