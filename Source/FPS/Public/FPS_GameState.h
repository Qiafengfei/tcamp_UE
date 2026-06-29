// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FPS_GameState.generated.h"

/** 游戏进行阶段 */
UENUM(BlueprintType)
enum class EMatchState : uint8
{
	Waiting		UMETA(DisplayName = "Waiting"),
	Playing		UMETA(DisplayName = "Playing"),
	Ended		UMETA(DisplayName = "Ended")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEndedSignature, int32, WinningTeam);

/**
 * FPS 游戏状态 - 管理队伍分数、比赛计时、胜利条件
 * 在服务器上运行核心逻辑，通过 Replication 同步到所有客户端
 */
UCLASS()
class FPS_API AFPS_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFPS_GameState();

	// ========== 属性 ==========

	/** 红队（Team 0）分数 */
	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, BlueprintReadOnly, Category = "Score")
	int32 Team0Score;

	/** 蓝队（Team 1）分数 */
	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, BlueprintReadOnly, Category = "Score")
	int32 Team1Score;

	/** 胜利所需分数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Match")
	int32 ScoreLimit;

	/** 已击杀 AI 数量（Replicated 给客户端显示计分板） */
	UPROPERTY(ReplicatedUsing = OnRep_AIKillCount, BlueprintReadOnly, Category = "Score")
	int32 KilledAICount;

	/** AI 总数量 */
	UPROPERTY(ReplicatedUsing = OnRep_AIKillCount, BlueprintReadOnly, Category = "Score")
	int32 TotalAICount;

	/** 比赛限时（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Match")
	float TimeLimit;

	/** 剩余时间（秒） */
	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, BlueprintReadOnly, Category = "Match")
	float RemainingTime;

	/** 比赛阶段 */
	UPROPERTY(ReplicatedUsing = OnRep_MatchStateChanged, BlueprintReadOnly, Category = "Match")
	EMatchState MatchState;

	/** 胜利队伍（-1=未决出, 0=红队, 1=蓝队） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 WinningTeam;

	/** 比赛结束委托（用于 UI 绑定） */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMatchEndedSignature OnMatchEnded;

	// ========== 函数 ==========

	/** 【Server】为指定队伍加分 */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddTeamScore(int32 TeamIndex, int32 Amount);

	/** 【Server】检查是否达到胜利条件 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void CheckWinCondition();

	/** 【Server】开始比赛倒计时 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatchTimer();

	/** 【Server】结束比赛 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void EndMatch(int32 InWinningTeam);

	/** 【Multicast】在所有客户端上通知比赛结束 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Match")
	void Multicast_OnMatchEnded(int32 InWinningTeam);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 比赛倒计时 Timer Handle */
	FTimerHandle MatchTimerHandle;

	/** 每1秒更新倒计时 */
	void UpdateMatchTimer();

	/** 队伍分数变化时 */
	UFUNCTION()
	void OnRep_TeamScores();

	/** 倒计时变化时 */
	UFUNCTION()
	void OnRep_RemainingTime();

	/** 比赛阶段变化时 */
	UFUNCTION()
	void OnRep_MatchStateChanged();

	/** 击杀数变化时 */
	UFUNCTION()
	void OnRep_AIKillCount();
};
