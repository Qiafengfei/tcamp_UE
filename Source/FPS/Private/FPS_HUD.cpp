#include "FPS_HUD.h"
#include "FPS_GameState.h"
#include "FPS_PlayerState.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"

AFPS_HUD::AFPS_HUD()
{
	bDrawScoreboard = false;
}

void AFPS_HUD::DrawHUD()
{
	Super::DrawHUD();
	if (bDrawScoreboard)
	{
		DrawScoreboard();
	}
}

void AFPS_HUD::ShowScoreboardHUD()
{
	bDrawScoreboard = true;
}

void AFPS_HUD::HideScoreboardHUD()
{
	bDrawScoreboard = false;
}

void AFPS_HUD::DrawText(const FString& Text, const FColor& Color, float X, float Y, float Scale)
{
	if (!Canvas) return;
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!Font) return;

	FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	TextItem.bOutlined = true;
	TextItem.OutlineColor = FLinearColor::Black;
	TextItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TextItem);
}

// ─── 核心：绘制计分板 ───────────────────────────────

void AFPS_HUD::DrawScoreboard()
{
	if (!Canvas) return;

	AFPS_GameState* GS = GetWorld() ? GetWorld()->GetGameState<AFPS_GameState>() : nullptr;
	if (!GS) return;

	const float CX = Canvas->SizeX * 0.5f;   // 屏幕中心 X
	const float CY = Canvas->SizeY * 0.5f;   // 屏幕中心 Y
	const float LineH = 26.0f;                // 行高

	// 半透明背景
	float PanelW = 500.0f;
	float PanelH = 300.0f;
	float PanelX = CX - PanelW * 0.5f;
	float PanelY = CY - PanelH * 0.5f;
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), PanelX, PanelY, PanelW, PanelH);

	float Y = PanelY + 20.0f;
	float LX = PanelX + 20.0f;  // 左缩进

	// ── 标题 ──
	DrawText(TEXT("========= 计分板 ========="), FColor::White, LX, Y, 1.1f);
	Y += LineH + 8.0f;

	// ── 消灭进度 ──
	DrawText(FString::Printf(TEXT("已消灭: %d / %d"), GS->KilledAICount, GS->TotalAICount), FColor::Yellow, LX, Y);
	Y += LineH;

	// ── 倒计时 ──
	int32 Minutes = FMath::FloorToInt(GS->RemainingTime / 60.0f);
	int32 Seconds = FMath::FloorToInt(GS->RemainingTime) % 60;
	DrawText(FString::Printf(TEXT("剩余时间: %02d:%02d"), Minutes, Seconds), FColor::Orange, LX, Y);
	Y += LineH;

	// ── 比分 ──
	DrawText(FString::Printf(TEXT("红队 %d  :  %d 蓝队"), GS->Team0Score, GS->Team1Score), FColor::Cyan, LX, Y);
	Y += LineH + 8.0f;

	// ── 表头 ──
	DrawText(TEXT("玩家                       击杀    死亡"), FColor::Silver, LX, Y);
	Y += LineH;
	DrawText(TEXT("── ────────────────────────── ──── ────"), FColor::Silver, LX, Y);
	Y += 4.0f;

	// ── 玩家列表（按击杀降序） ──
	TArray<AFPS_PlayerState*> SortedPlayers;
	for (APlayerState* PS : GS->PlayerArray)
	{
		AFPS_PlayerState* FPS_PS = Cast<AFPS_PlayerState>(PS);
		if (FPS_PS) SortedPlayers.Add(FPS_PS);
	}
	SortedPlayers.Sort([](const AFPS_PlayerState& A, const AFPS_PlayerState& B) {
		return A.PlayerKills > B.PlayerKills;
	});

	for (AFPS_PlayerState* FPS_PS : SortedPlayers)
	{
		FColor TeamColor = (FPS_PS->PlayerTeam == 0) ? FColor::Red : FColor::Blue;
		FString TeamMark = (FPS_PS->PlayerTeam == 0) ? TEXT("[红]") : TEXT("[蓝]");
		FString Line = FString::Printf(TEXT("%s %-20s %5d   %5d"),
			*TeamMark,
			*FPS_PS->GetPlayerNickname(),
			FPS_PS->PlayerKills,
			FPS_PS->PlayerDeaths);

		DrawText(Line, TeamColor, LX, Y);
		Y += LineH;
	}
}
