// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FPS_GameMode.generated.h"

class AFPS_GameState;
class AFPS_PlayerState;

/**
 * FPS 游戏模式 - 管理团队分配、击杀计分、AI生成、比赛生命周期
 */
UCLASS()
class FPS_API AFPS_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AFPS_GameMode();

	// ========== 属性 ==========

	/** AI 生成数量（每队） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	int32 AICountPerTeam;

	/** AI 类（指向 BP_AI 蓝图） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	TSubclassOf<class APawn> AIPawnClass;

	/** 团队重生点数组 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawns")
	TArray<AActor*> Team0SpawnPoints;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawns")
	TArray<AActor*> Team1SpawnPoints;

	/** AI 生成点 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawns")
	TArray<AActor*> AISpawnPoints;

	// ========== 函数 ==========

	/** 生成 AI */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SpawnAITeams();

	/** 扫描地图上已有的 AI 并绑定计分事件 */
	void BindExistingAI();

	/** 统一计分入口 */
	void TryScoreKill(APawn* VictimPawn, AController* KillerController, AController* VictimController = nullptr);

	/** 广播消灭进度到所有客户端 */
	void BroadcastKillCount();

	/** AI 控制器上报死亡 */
	void OnAIDeath(class AFPS_AI_Controller* AICtrl);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 玩家登录时分配队伍 */
	virtual void OnPostLogin(AController* NewPlayer) override;

	/** 玩家退出时处理 */
	virtual void Logout(AController* Exiting) override;

	/** 在指定队伍生成玩家 */
	void SpawnPlayerForTeam(AController* NewPlayer, int32 TeamIndex);

	/** 获取指定队伍的生成点 */
	AActor* GetSpawnPointForTeam(int32 TeamIndex);

	/** 获取 AI 生成点 */
	AActor* GetAISpawnPoint();

	/** 分配队伍（均衡负载） */
	int32 AssignTeam();

private:
	/** 监听伤害事件（追踪最后攻击者） */
	UFUNCTION()
	void OnPawnTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	/** 监听 Pawn 销毁事件 */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** 绑定伤害和死亡监听到一个 Pawn */
	void BindPawnEvents(APawn* Pawn);

	/** 重生玩家 */
	void RespawnPlayer(AController* Controller);

	/** 追踪每个 Pawn 的最后击杀者 */
	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<AController>> LastDamageInstigatorMap;

	/** AI 血量追踪 */
	TMap<TWeakObjectPtr<APawn>, float> AIPawnHealthMap;

	/** AI 最大血量 */
	float AIMaxHealth;

	/** 剩余敌人数量（杀掉所有敌人 = 胜利） */
	int32 RemainingAI;

	/** AI Pawn → Controller 映射（用于 Pawn 销毁后找回 Controller） */
	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<AController>> SpawnedAIControllerMap;

	/** AI 生成时间（防刷出生即死不算分） */
	TMap<TWeakObjectPtr<APawn>, float> AISpawnTimeMap;

	/** 比赛重启计时器 */
	FTimerHandle RestartTimerHandle;
	void RestartMatch();
};
