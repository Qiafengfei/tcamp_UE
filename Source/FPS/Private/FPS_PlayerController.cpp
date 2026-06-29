#include "FPS_PlayerController.h"
#include "FPS_PlayerState.h"
#include "FPS_GameInstance.h"
#include "FPS_GameState.h"
#include "FPS_HUD.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AFPS_PlayerController::AFPS_PlayerController()
{
	bShowMouseCursor = false;
	bReplicates = true;
}

void AFPS_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[FPS] AFPS_PlayerController::BeginPlay - %s"), *GetName());

	CachedGameState = GetWorld() ? GetWorld()->GetGameState<AFPS_GameState>() : nullptr;
	if (CachedGameState)
	{
		CachedGameState->OnMatchEnded.AddDynamic(this, &AFPS_PlayerController::OnMatchEnded);
	}

	InitGameUI();
}

void AFPS_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedGameState)
	{
		CachedGameState->OnMatchEnded.RemoveDynamic(this, &AFPS_PlayerController::OnMatchEnded);
	}
	Super::EndPlay(EndPlayReason);
}

void AFPS_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("[FPS] SetupInputComponent - Binding Tab/F1/F2"));
		InputComponent->BindKey(FKey("Tab"), IE_Pressed, this, &AFPS_PlayerController::ShowScoreboard);
		InputComponent->BindKey(FKey("Tab"), IE_Released, this, &AFPS_PlayerController::HideScoreboard);
		InputComponent->BindKey(FKey("F1"), IE_Pressed, this, &AFPS_PlayerController::OnKeyHost);
		InputComponent->BindKey(FKey("F2"), IE_Pressed, this, &AFPS_PlayerController::OnKeyJoin);
	}
}

void AFPS_PlayerController::OnKeyHost()
{
	UE_LOG(LogTemp, Log, TEXT("[FPS] F1 pressed - Hosting LAN game"));
	UFPS_GameInstance* GI = GetGameInstance<UFPS_GameInstance>();
	if (GI) GI->HostLAN(TEXT("L_Playground"));
}

void AFPS_PlayerController::OnKeyJoin()
{
	UE_LOG(LogTemp, Log, TEXT("[FPS] F2 pressed - Joining LAN game"));
	UFPS_GameInstance* GI = GetGameInstance<UFPS_GameInstance>();
	if (GI) GI->JoinLAN(TEXT("127.0.0.1"));
}

void AFPS_PlayerController::ShowScoreboard()
{
	UE_LOG(LogTemp, Log, TEXT("[FPS] ShowScoreboard CALLED!"));
	if (AFPS_HUD* HUD = GetHUD<AFPS_HUD>())
	{
		HUD->ShowScoreboardHUD();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPS] HUD is null or wrong class!"));
	}
}

void AFPS_PlayerController::HideScoreboard()
{
	UE_LOG(LogTemp, Log, TEXT("[FPS] HideScoreboard CALLED!"));
	if (AFPS_HUD* HUD = GetHUD<AFPS_HUD>())
	{
		HUD->HideScoreboardHUD();
	}
}

void AFPS_PlayerController::OnMatchEnded(int32 WinningTeam)
{
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	ShowScoreboard();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(19, 30.0f, FColor::Green,
			FString::Printf(TEXT("比赛结束！队伍 %d 获胜"), WinningTeam));
	}
}

void AFPS_PlayerController::Client_ShowKillCount_Implementation(int32 Killed, int32 Total)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(0, 60.0f, FColor::Cyan,
			FString::Printf(TEXT("消灭敌人: %d / %d"), Killed, Total));
	}
}
