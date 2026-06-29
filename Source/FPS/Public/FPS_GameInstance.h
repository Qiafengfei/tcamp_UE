// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FPS_GameInstance.generated.h"

/**
 * FPS 游戏实例 - LAN 联机（仅控制台命令，无 UI 依赖）
 *
 * 用法（按 ~ 打开控制台输入）：
 *   HostLAN L_Playground   → 创建 LAN 主机
 *   JoinLAN 192.168.1.100  → 加入 LAN 游戏
 */
UCLASS()
class FPS_API UFPS_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFPS_GameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	/** 创建 LAN 主机 */
	void HostLAN(const FString& MapName);

	/** 通过 IP 加入 */
	void JoinLAN(const FString& IPAddress);

private:
	void ConsoleHostLAN(const TArray<FString>& Args);
	void ConsoleJoinLAN(const TArray<FString>& Args);

	IConsoleObject* CmdHost;
	IConsoleObject* CmdJoin;
};
