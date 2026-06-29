// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPS_PlayerState.generated.h"

/**
 * FPS 玩家状态 - 管理个人击杀/死亡/队伍信息
 * 用于计分板显示和团队分配
 */
UCLASS()
class FPS_API AFPS_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFPS_PlayerState();

	// ========== 属性 ==========

	/** 玩家所属队伍（0=红队, 1=蓝队） */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerTeam, BlueprintReadWrite, Category = "Team")
	int32 PlayerTeam;

	/** 击杀数 */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerStats, BlueprintReadOnly, Category = "Stats")
	int32 PlayerKills;

	/** 死亡数 */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerStats, BlueprintReadOnly, Category = "Stats")
	int32 PlayerDeaths;

	// ========== 函数 ==========

	/** 【Server】增加击杀数 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddKill();

	/** 【Server】增加死亡数 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddDeath();

	/** 重置统计数据 */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetStats();

	/** 获取玩家昵称 */
	UFUNCTION(BlueprintCallable, Category = "Info")
	FString GetPlayerNickname() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_PlayerTeam();

	UFUNCTION()
	void OnRep_PlayerStats();
};
