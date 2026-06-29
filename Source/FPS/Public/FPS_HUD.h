#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPS_HUD.generated.h"

/**
 * FPS 计分板 HUD —— 用 Canvas 直接绘制，不依赖 UMG / DebugMessage
 */
UCLASS()
class FPS_API AFPS_HUD : public AHUD
{
	GENERATED_BODY()

public:
	AFPS_HUD();

	/** 绘制 HUD（每帧调用） */
	virtual void DrawHUD() override;

	/** 显示计分板 */
	void ShowScoreboardHUD();

	/** 隐藏计分板 */
	void HideScoreboardHUD();

	/** 是否正在显示计分板 */
	bool IsScoreboardVisible() const { return bDrawScoreboard; }

private:
	bool bDrawScoreboard;

	void DrawScoreboard();

	/** 简化的文字绘制辅助 */
	void DrawText(const FString& Text, const FColor& Color, float X, float Y, float Scale = 1.0f);
};
