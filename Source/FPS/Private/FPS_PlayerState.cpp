// Fill out your copyright notice in the Description page of Project Settings.

#include "FPS_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

AFPS_PlayerState::AFPS_PlayerState()
{
	PlayerTeam = 0;
	PlayerKills = 0;
	PlayerDeaths = 0;

	bReplicates = true;
	bAlwaysRelevant = true;
}

void AFPS_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFPS_PlayerState, PlayerTeam, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_PlayerState, PlayerKills, COND_None);
	DOREPLIFETIME_CONDITION(AFPS_PlayerState, PlayerDeaths, COND_None);
}

void AFPS_PlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void AFPS_PlayerState::AddKill()
{
	if (!HasAuthority()) return;
	PlayerKills++;
	// 更新继承的 Score 属性（用于在线子系统排名）
	SetScore(PlayerKills * 100.0f);
	OnRep_PlayerStats();
}

void AFPS_PlayerState::AddDeath()
{
	if (!HasAuthority()) return;
	PlayerDeaths++;
	OnRep_PlayerStats();
}

void AFPS_PlayerState::ResetStats()
{
	if (!HasAuthority()) return;
	PlayerKills = 0;
	PlayerDeaths = 0;
	SetScore(0.0f);
	OnRep_PlayerStats();
}

FString AFPS_PlayerState::GetPlayerNickname() const
{
	// 优先使用 PlayerName，否则返回默认
	return GetPlayerName().IsEmpty() ? TEXT("Player") : GetPlayerName();
}

// ─── OnRep ──────────────────────────────────────

void AFPS_PlayerState::OnRep_PlayerStats()
{
}

void AFPS_PlayerState::OnRep_PlayerTeam()
{
}
