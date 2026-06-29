// Fill out your copyright notice in the Description page of Project Settings.

#include "FPS_GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

static FString ResolveMap(const FString& MapName)
{
	if (MapName.StartsWith(TEXT("/"))) return MapName;
	return FString::Printf(TEXT("/Game/InfimaGames/FreeFPSTemplate/Maps/%s"), *MapName);
}

// ============================================================

UFPS_GameInstance::UFPS_GameInstance()
	: CmdHost(nullptr), CmdJoin(nullptr)
{
}

void UFPS_GameInstance::Init()
{
	Super::Init();

	CmdHost = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("HostLAN"),
		TEXT("创建 LAN 主机。用法: HostLAN <地图名>  示例: HostLAN L_Playground"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UFPS_GameInstance::ConsoleHostLAN),
		ECVF_Default
	);

	CmdJoin = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("JoinLAN"),
		TEXT("加入 LAN 游戏。用法: JoinLAN <主机IP>  示例: JoinLAN 192.168.1.100"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &UFPS_GameInstance::ConsoleJoinLAN),
		ECVF_Default
	);

	UE_LOG(LogTemp, Log, TEXT("[FPS] 联机命令已注册: HostLAN <图>, JoinLAN <IP>"));
}

void UFPS_GameInstance::Shutdown()
{
	if (CmdHost) { IConsoleManager::Get().UnregisterConsoleObject(CmdHost); CmdHost = nullptr; }
	if (CmdJoin) { IConsoleManager::Get().UnregisterConsoleObject(CmdJoin); CmdJoin = nullptr; }
	Super::Shutdown();
}

// ============================================================

void UFPS_GameInstance::HostLAN(const FString& MapName)
{
	UWorld* World = GetWorld();
	if (!World) { UE_LOG(LogTemp, Error, TEXT("[FPS] 无 World")); return; }

	FString URL = ResolveMap(MapName) + TEXT("?listen");
	UE_LOG(LogTemp, Log, TEXT("[FPS] 创建主机 → %s"), *URL);
	World->ServerTravel(URL);
}

void UFPS_GameInstance::JoinLAN(const FString& IPAddress)
{
	APlayerController* PC = GetFirstLocalPlayerController(GetWorld());
	if (!PC) { UE_LOG(LogTemp, Error, TEXT("[FPS] 无 PlayerController")); return; }

	FString URL = IPAddress.Contains(TEXT(":")) ? IPAddress : (IPAddress + TEXT(":7777"));
	UE_LOG(LogTemp, Log, TEXT("[FPS] 加入游戏 → %s"), *URL);
	PC->ClientTravel(URL, TRAVEL_Absolute);
}

// ============================================================

void UFPS_GameInstance::ConsoleHostLAN(const TArray<FString>& Args)
{
	if (Args.Num() < 1) { UE_LOG(LogTemp, Warning, TEXT("[FPS] 用法: HostLAN <地图名>")); return; }
	HostLAN(Args[0]);
}

void UFPS_GameInstance::ConsoleJoinLAN(const TArray<FString>& Args)
{
	if (Args.Num() < 1) { UE_LOG(LogTemp, Warning, TEXT("[FPS] 用法: JoinLAN <IP>")); return; }
	JoinLAN(Args[0]);
}
