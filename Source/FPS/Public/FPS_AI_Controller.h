// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "FPS_AI_Controller.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardData;

/**
 * FPS AI 控制器 - 管理 AI 感知、行为树、攻击逻辑
 */
UCLASS()
class FPS_API AFPS_AI_Controller : public AAIController
{
	GENERATED_BODY()

public:
	AFPS_AI_Controller(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 攻击范围 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	float AttackRange;

	/** 攻击间隔（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	float AttackCooldown;

	/** 单次攻击伤害 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	float AttackDamage;

	/** 行为树资产 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	/** 黑板资产 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBlackboardData* BlackboardAsset;

	// ========== 感知 ==========

	/** 感知组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Perception")
	UAIPerceptionComponent* PerceptionComp;

	/** 当感知到目标时 */
	UFUNCTION()
	void OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus);

	/** 获取当前最近的敌人 */
	AActor* GetNearestEnemy() const;

	/** 执行攻击 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAttack(AActor* Target);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 攻击冷却计时器 */
	FTimerHandle AttackCooldownTimer;

	/** 是否可以攻击 */
	bool bCanAttack;

	/** 攻击冷却回调 */
	void OnAttackCooldownEnd();

	/** 初始化感知系统 */
	void SetupPerception();

	/** 最后一个感知到的敌人 */
	TWeakObjectPtr<AActor> LastPerceivedEnemy;

public:
	/** 是否已上报死亡（防止重复上报） */
	bool bDeathReported;

	/** 最后攻击者（由 GameMode 在伤害时设置） */
	TWeakObjectPtr<AController> LastAttacker;
};
