#include "FPS_GameMode.h"
#include "FPS_GameState.h"
#include "FPS_PlayerState.h"
#include "FPS_PlayerController.h"
#include "FPS_AI_Controller.h"
#include "FPS_HUD.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AFPS_GameMode::AFPS_GameMode()
{
	GameStateClass = AFPS_GameState::StaticClass();
	PlayerStateClass = AFPS_PlayerState::StaticClass();
	PlayerControllerClass = AFPS_PlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnFinder(TEXT("/Game/InfimaGames/FreeFPSTemplate/Core/BP_Character.BP_Character_C"));
	if (PlayerPawnFinder.Succeeded())
		DefaultPawnClass = PlayerPawnFinder.Class;

	AICountPerTeam = 3;
	AIMaxHealth = 50.0f;
	RemainingAI = 0;

	static ConstructorHelpers::FClassFinder<APawn> AIPawnFinder(TEXT("/Game/BP_AI.BP_AI_C"));
	if (AIPawnFinder.Succeeded())
		AIPawnClass = AIPawnFinder.Class;

	HUDClass = AFPS_HUD::StaticClass();
	PrimaryActorTick.bCanEverTick = false;
	bUseSeamlessTravel = true;
}

void AFPS_GameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	AFPS_GameState* GS = GetGameState<AFPS_GameState>();
	if (GS) GS->StartMatchTimer();

	// 不改了你放的 AI，直接绑定所有已存在的 BP_AI
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &AFPS_GameMode::BindExistingAI, 0.5f, false);
}

void AFPS_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RestartTimerHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(RestartTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AFPS_GameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);
	if (!HasAuthority()) return;

	int32 TeamIndex = AssignTeam();
	AFPS_PlayerState* PS = NewPlayer->GetPlayerState<AFPS_PlayerState>();
	if (PS)
	{
		PS->PlayerTeam = TeamIndex;
		PS->ResetStats();
	}
	SpawnPlayerForTeam(NewPlayer, TeamIndex);
}

void AFPS_GameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

int32 AFPS_GameMode::AssignTeam()
{
	int32 T0 = 0, T1 = 0;
	for (auto It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (C && C->IsPlayerController())
		{
			AFPS_PlayerState* PS = C->GetPlayerState<AFPS_PlayerState>();
			if (PS)
			{
				if (PS->PlayerTeam == 0) T0++;
				else if (PS->PlayerTeam == 1) T1++;
			}
		}
	}
	return (T0 <= T1) ? 0 : 1;
}

AActor* AFPS_GameMode::GetSpawnPointForTeam(int32 TeamIndex)
{
	TArray<AActor*> Points;
	if (TeamIndex == 0 && Team0SpawnPoints.Num() > 0) Points = Team0SpawnPoints;
	else if (TeamIndex == 1 && Team1SpawnPoints.Num() > 0) Points = Team1SpawnPoints;
	if (Points.Num() > 0) return Points[FMath::RandRange(0, Points.Num() - 1)];

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() > 0) return PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
	return nullptr;
}

AActor* AFPS_GameMode::GetAISpawnPoint()
{
	// 收集玩家位置
	TArray<FVector> PlayerLocs;
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
			PlayerLocs.Add(PC->GetPawn()->GetActorLocation());
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	// 找离玩家最远的 PlayerStart
	AActor* Best = nullptr;
	float BestDist = -1;
	for (AActor* PS : PlayerStarts)
	{
		FVector Loc = PS->GetActorLocation();
		float MinD = FLT_MAX;
		for (const FVector& PL : PlayerLocs)
			MinD = FMath::Min(MinD, FVector::Dist(Loc, PL));
		if (MinD > BestDist) { BestDist = MinD; Best = PS; }
	}
	if (Best) return Best;
	if (PlayerStarts.Num() > 0) return PlayerStarts[0];
	return nullptr;
}

void AFPS_GameMode::SpawnPlayerForTeam(AController* NewPlayer, int32 TeamIndex)
{
	if (!HasAuthority() || !NewPlayer || !DefaultPawnClass) return;

	AActor* SpawnPoint = GetSpawnPointForTeam(TeamIndex);
	if (!SpawnPoint) SpawnPoint = FindPlayerStart(NewPlayer);

	FVector SpawnLoc = SpawnPoint ? SpawnPoint->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRot = SpawnPoint ? SpawnPoint->GetActorRotation() : FRotator::ZeroRotator;

	APawn* Old = NewPlayer->GetPawn();
	if (Old)
	{
		Old->OnDestroyed.RemoveDynamic(this, &AFPS_GameMode::OnPawnDestroyed);
		Old->OnTakeAnyDamage.RemoveDynamic(this, &AFPS_GameMode::OnPawnTakeDamage);
		Old->Destroy();
	}

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, SpawnLoc, SpawnRot, P);
	if (NewPawn)
	{
		NewPlayer->Possess(NewPawn);
		BindPawnEvents(NewPawn);
	}
}

// ─── 扫描地图上已有的 AI 并绑定计分 ──────────────

// SpawnAITeams 不再使用（改用 BindExistingAI），但保留空实现供 UHT 引用
void AFPS_GameMode::SpawnAITeams()
{
}

void AFPS_GameMode::BindExistingAI()
{
	if (!HasAuthority()) return;
	if (!AIPawnClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPS] AIPawnClass 为空，无法绑定 AI"));
		return;
	}

	TArray<AActor*> FoundAIs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIPawnClass, FoundAIs);

	RemainingAI = 0;
	for (AActor* Actor : FoundAIs)
	{
		APawn* AIPawn = Cast<APawn>(Actor);
		if (!AIPawn) continue;

		BindPawnEvents(AIPawn);
		AIPawnHealthMap.Add(AIPawn, AIMaxHealth);
		AISpawnTimeMap.Add(AIPawn, GetWorld()->GetTimeSeconds());
		RemainingAI++;

		UE_LOG(LogTemp, Log, TEXT("[FPS] 绑定 AI %d: %s"), RemainingAI, *AIPawn->GetName());
	}

	AFPS_GameState* GS = GetGameState<AFPS_GameState>();
	if (GS)
	{
		GS->ScoreLimit = RemainingAI;
		GS->TotalAICount = RemainingAI;
		GS->KilledAICount = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[FPS] 共绑定 %d 个地图 AI"), RemainingAI);

	// 广播初始杀敌数
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFPS_PlayerController* PC = Cast<AFPS_PlayerController>(It->Get());
		if (PC) PC->Client_ShowKillCount(0, RemainingAI);
	}
}

// ─── 绑定事件 ───────────────────────────────────

void AFPS_GameMode::BindPawnEvents(APawn* Pawn)
{
	if (!Pawn) return;
	Pawn->OnTakeAnyDamage.AddDynamic(this, &AFPS_GameMode::OnPawnTakeDamage);
	Pawn->OnDestroyed.AddDynamic(this, &AFPS_GameMode::OnPawnDestroyed);
}

// ─── 伤害/击杀处理 ──────────────────────────────

void AFPS_GameMode::OnPawnTakeDamage(AActor* DamagedActor, float Damage,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!HasAuthority()) return;
	APawn* DamagedPawn = Cast<APawn>(DamagedActor);
	if (!DamagedPawn) return;

	if (InstigatedBy)
	{
		LastDamageInstigatorMap.Add(DamagedPawn, InstigatedBy);
		// 记录到 AI 控制器（Tick 检测死亡时用）
		if (AFPS_AI_Controller* AIC = Cast<AFPS_AI_Controller>(DamagedPawn->GetController()))
		{
			AIC->LastAttacker = InstigatedBy;
		}
	}

	if (!DamagedPawn->IsPlayerControlled())
	{
		float* HPPtr = AIPawnHealthMap.Find(DamagedPawn);
		float NewHP = (HPPtr ? *HPPtr : AIMaxHealth) - Damage;
		AIPawnHealthMap.Add(DamagedPawn, NewHP);

		if (NewHP <= 0.0f)
		{
			// AI控制器Tick不会再上报
			if (AFPS_AI_Controller* AIC = Cast<AFPS_AI_Controller>(DamagedPawn->GetController()))
			{
				AIC->bDeathReported = true;
				}

			DamagedPawn->OnDestroyed.RemoveDynamic(this, &AFPS_GameMode::OnPawnDestroyed);
			AIPawnHealthMap.Remove(DamagedPawn);
			TryScoreKill(DamagedPawn, InstigatedBy);
			DamagedPawn->Destroy();
		}
	}
}

// ─── AI 死亡 ────────────────────────────────────
// 注意：标注死亡由 AI 控制器的 Tick 检测完成（OnAIDeath）
// OnPawnDestroyed 只负责清理追踪数据

void AFPS_GameMode::OnPawnDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority()) return;
	APawn* Pawn = Cast<APawn>(DestroyedActor);
	if (!Pawn) return;

	if (SpawnedAIControllerMap.Contains(Pawn))
	{
		SpawnedAIControllerMap.Remove(Pawn);
	}
	AIPawnHealthMap.Remove(Pawn);
	AISpawnTimeMap.Remove(Pawn);
	LastDamageInstigatorMap.Remove(Pawn);
}

void AFPS_GameMode::OnAIDeath(AFPS_AI_Controller* AICtrl)
{
	if (!HasAuthority() || !AICtrl) return;
	if (AICtrl->bDeathReported) return; // 防止重复
	AICtrl->bDeathReported = true;

	AFPS_GameState* GS = GetGameState<AFPS_GameState>();
	if (GS && GS->MatchState == EMatchState::Ended) return;

	// 找击杀者
	AController* Killer = AICtrl->LastAttacker.Get();
	AICtrl->LastAttacker = nullptr;

	AFPS_PlayerState* VictimPS = AICtrl->GetPlayerState<AFPS_PlayerState>();

	if (Killer && Killer != AICtrl)
	{
		AFPS_PlayerState* KillerPS = Killer->GetPlayerState<AFPS_PlayerState>();
		if (KillerPS && VictimPS)
		{
			KillerPS->AddKill();
			VictimPS->AddDeath();
			if (GS) GS->AddTeamScore(KillerPS->PlayerTeam, 1);
		}
	}
	else if (VictimPS)
	{
		VictimPS->AddDeath();
	}

	// 减少敌人数 + 广播
	RemainingAI--;
	if (GS) GS->KilledAICount++;
	BroadcastKillCount();

	if (RemainingAI <= 0 && GS && GS->MatchState != EMatchState::Ended)
	{
		int32 WinTeam = -1;
		AFPS_PlayerState* KillerPS = Killer ? Killer->GetPlayerState<AFPS_PlayerState>() : nullptr;
		if (KillerPS) WinTeam = KillerPS->PlayerTeam;
		GS->EndMatch(WinTeam);
	}
}

// ─── 计分 + 广播 ───────────────────────────────

void AFPS_GameMode::BroadcastKillCount()
{
	AFPS_GameState* GS = GetGameState<AFPS_GameState>();
	int32 Total = GS ? GS->ScoreLimit : 6;
	int32 Killed = Total - RemainingAI;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFPS_PlayerController* PC = Cast<AFPS_PlayerController>(It->Get());
		if (PC) PC->Client_ShowKillCount(Killed, Total);
	}
}

void AFPS_GameMode::TryScoreKill(APawn* VictimPawn, AController* KillerController, AController* VictimController)
{
	if (!HasAuthority() || !VictimPawn) return;

	if (!VictimController) VictimController = VictimPawn->GetController();
	if (!VictimController) return;

	AFPS_PlayerState* VictimPS = VictimController->GetPlayerState<AFPS_PlayerState>();
	AFPS_GameState* GS = GetGameState<AFPS_GameState>();

	// ── 玩家击杀另一个玩家（有明确杀人者） ──
	if (KillerController && KillerController != VictimController)
	{
		AFPS_PlayerState* KillerPS = KillerController->GetPlayerState<AFPS_PlayerState>();
		if (KillerPS && VictimPS)
		{
			KillerPS->AddKill();
			VictimPS->AddDeath();
			if (GS) GS->AddTeamScore(KillerPS->PlayerTeam, 1);
		}

		// 如果死的是玩家 → 重生
		if (VictimController->IsPlayerController())
		{
			RespawnPlayer(VictimController);
			return;
		}
	}
	else if (VictimPS)
	{
		// 没有指定杀人者（如子弹 ApplyDamage 没传 InstigatedBy）
		VictimPS->AddDeath();
	}

	// ── AI 死亡（不管有没有杀人者都计） ──
	if (!VictimController->IsPlayerController())
	{
		RemainingAI--;
		if (GS) GS->KilledAICount++;
		BroadcastKillCount();

		if (RemainingAI <= 0 && GS && GS->MatchState != EMatchState::Ended)
		{
			int32 WinTeam = -1;
			if (KillerController)
			{
				AFPS_PlayerState* KillerPS = KillerController->GetPlayerState<AFPS_PlayerState>();
				if (KillerPS) WinTeam = KillerPS->PlayerTeam;
			}
			GS->EndMatch(WinTeam);
		}
	}
}

// ─── 重生 ───────────────────────────────────────

void AFPS_GameMode::RespawnPlayer(AController* Controller)
{
	if (!HasAuthority() || !Controller) return;
	FTimerDelegate Del = FTimerDelegate::CreateLambda([this, Controller]()
	{
		if (Controller && Controller->GetPawn() == nullptr)
		{
			AFPS_PlayerState* PS = Controller->GetPlayerState<AFPS_PlayerState>();
			if (PS) SpawnPlayerForTeam(Controller, PS->PlayerTeam);
		}
	});
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, Del, 3.0f, false);
}

void AFPS_GameMode::RestartMatch()
{
	if (!HasAuthority()) return;
	GetWorld()->ServerTravel(TEXT("?restart"), true);
}
