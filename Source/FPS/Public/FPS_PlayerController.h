#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPS_PlayerController.generated.h"

UCLASS()
class FPS_API AFPS_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFPS_PlayerController();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowScoreboard();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideScoreboard();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void InitGameUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void ShowVictoryScreen(int32 WinningTeam, int32 RedScore, int32 BlueScore);

	/** 唯一有用的 RPC：客户端显示杀敌数 */
	UFUNCTION(Client, Reliable, Category = "Score")
	void Client_ShowKillCount(int32 Killed, int32 Total);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnKeyHost();

	UFUNCTION()
	void OnKeyJoin();

	UFUNCTION()
	void OnMatchEnded(int32 WinningTeam);

	UPROPERTY()
	class AFPS_GameState* CachedGameState;
};
