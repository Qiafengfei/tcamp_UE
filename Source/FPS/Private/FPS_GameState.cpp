// Fill out your copyright notice in the Description page of Project Settings.

#include "FPS_GameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFPS_GameState::AFPS_GameState()
{
	Team0Score = 0;
	Team1Score = 0;
	ScoreLimit = 30;
	TimeLimit = 300.0f;
	KilledAICount = 0;
	TotalAICount = 6;
	RemainingTime = TimeLimit;
	MatchState = EMatchState::Waiting;
	WinningTeam = -1;

	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AFPS_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFPS_GameState, Team0Score, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, Team1Score, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, RemainingTime, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, MatchState, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, WinningTeam, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, KilledAICount, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_GameState, TotalAICount, COND_None);
}

void AFPS_GameState::BeginPlay()
{
	Super::BeginPlay();
}

void AFPS_GameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

// ─── 队伍加分 ───────────────────────────────────

void AFPS_GameState::AddTeamScore(int32 TeamIndex, int32 Amount)
{
	if (!HasAuthority()) return;

	if (TeamIndex == 0)
		Team0Score += Amount;
	else if (TeamIndex == 1)
		Team1Score += Amount;
	else
		return;

	OnRep_TeamScores();
	CheckWinCondition();
}

// ─── 胜利检查 ───────────────────────────────────

void AFPS_GameState::CheckWinCondition()
{
	if (!HasAuthority()) return;
	if (MatchState == EMatchState::Ended) return;

	if (Team0Score >= ScoreLimit)
	{
		EndMatch(0);
	}
	else if (Team1Score >= ScoreLimit)
	{
		EndMatch(1);
	}
}

// ─── 计时器 ─────────────────────────────────────

void AFPS_GameState::StartMatchTimer()
{
	if (!HasAuthority()) return;

	MatchState = EMatchState::Playing;
	OnRep_MatchStateChanged();
	RemainingTime = TimeLimit;
	OnRep_RemainingTime();

	GetWorld()->GetTimerManager().SetTimer(MatchTimerHandle, this, &AFPS_GameState::UpdateMatchTimer, 1.0f, true);
}

void AFPS_GameState::UpdateMatchTimer()
{
	if (!HasAuthority()) return;
	if (MatchState != EMatchState::Playing) return;

	RemainingTime -= 1.0f;
	OnRep_RemainingTime();

	if (RemainingTime <= 0.0f)
	{
		if (Team0Score > Team1Score)
			EndMatch(0);
		else if (Team1Score > Team0Score)
			EndMatch(1);
		else
			EndMatch(0);
	}
}

// ─── 结束比赛 ───────────────────────────────────

void AFPS_GameState::EndMatch(int32 InWinningTeam)
{
	if (!HasAuthority()) return;
	if (MatchState == EMatchState::Ended) return;

	WinningTeam = InWinningTeam;
	MatchState = EMatchState::Ended;
	OnRep_MatchStateChanged();

	if (MatchTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchTimerHandle);
	}

	Multicast_OnMatchEnded(InWinningTeam);
}

void AFPS_GameState::Multicast_OnMatchEnded_Implementation(int32 InWinningTeam)
{
	WinningTeam = InWinningTeam;
	OnMatchEnded.Broadcast(InWinningTeam);

	UE_LOG(LogTemp, Log, TEXT("[FPS] Match Ended! Winning Team: %d  (Score: %d - %d)"),
		InWinningTeam, Team0Score, Team1Score);
}

// ─── OnRep ──────────────────────────────────────

void AFPS_GameState::OnRep_TeamScores()
{
	// 计分板通过 Client RPC 更新
}

void AFPS_GameState::OnRep_RemainingTime()
{
}

void AFPS_GameState::OnRep_MatchStateChanged()
{
}

void AFPS_GameState::OnRep_AIKillCount()
{
}
