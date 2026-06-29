// Fill out your copyright notice in the Description page of Project Settings.

#include "FPS_AI_Controller.h"
#include "FPS_PlayerState.h"
#include "FPS_GameMode.h"
#include "FPS_GameState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

AFPS_AI_Controller::AFPS_AI_Controller(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AttackRange = 2000.0f;
	AttackCooldown = 1.5f;
	AttackDamage = 20.0f;
	bCanAttack = true;
	bDeathReported = false;
	LastPerceivedEnemy = nullptr;

	// 创建感知组件
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*PerceptionComp);

	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // 每秒10次检查
}

void AFPS_AI_Controller::BeginPlay()
{
	Super::BeginPlay();
	SetupPerception();
}

void AFPS_AI_Controller::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 启动行为树
	if (BehaviorTreeAsset && BlackboardAsset)
	{
		UBlackboardComponent* BlackboardComp;
		if (UseBlackboard(BlackboardAsset, BlackboardComp))
		{
			RunBehaviorTree(BehaviorTreeAsset);
			UE_LOG(LogTemp, Log, TEXT("[FPS_AI] Behavior Tree started for %s"), *InPawn->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPS_AI] No BehaviorTree or Blackboard assigned! AI will use Tick-based logic."));
	}
}

void AFPS_AI_Controller::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// === 死亡检测：Pawn不见了 → 死了 → 上报 GameMode ===
	if (HasAuthority() && !bDeathReported && GetPawn() == nullptr)
	{
		bDeathReported = true;
		AFPS_GameState* GS = GetWorld()->GetGameState<AFPS_GameState>();
		if (GS && GS->MatchState != EMatchState::Ended)
		{
			if (AFPS_GameMode* GM = Cast<AFPS_GameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->OnAIDeath(this);
			}
		}
		return;
	}

	// 只有在没有行为树时才用 Tick 攻击逻辑
	if (BehaviorTreeAsset != nullptr && BlackboardAsset != nullptr)
		return;

	if (!HasAuthority()) return;

	// 找最近敌人
	AActor* Enemy = GetNearestEnemy();
	if (!Enemy)
	{
		// 没目标时巡逻或站立
		return;
	}

	// 更新黑板（如果有的话）
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (BB)
	{
		BB->SetValueAsObject(TEXT("TargetActor"), Enemy);
		BB->SetValueAsVector(TEXT("TargetLocation"), Enemy->GetActorLocation());
	}

	// 转向敌人
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(GetPawn()->GetActorLocation(), Enemy->GetActorLocation());
	GetPawn()->SetActorRotation(FRotator(0.f, LookAtRot.Yaw, 0.f));

	// 检查距离
	float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Enemy->GetActorLocation());
	if (Dist <= AttackRange && bCanAttack)
	{
		PerformAttack(Enemy);
	}
	else if (Dist > AttackRange)
	{
		// 靠近敌人 - 设置导航目标
		MoveToActor(Enemy, AttackRange * 0.5f, true, true, true, nullptr, true);
	}
}

// ─── 感知系统 ──────────────────────────────────

void AFPS_AI_Controller::SetupPerception()
{
	if (!PerceptionComp) return;

	// 创建视觉感知配置
	UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(this);
	if (SightConfig)
	{
		SightConfig->SightRadius = 5000.0f;
		SightConfig->LoseSightRadius = 6000.0f;
		SightConfig->PeripheralVisionAngleDegrees = 90.0f;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		SightConfig->SetMaxAge(5.0f);
		SightConfig->AutoSuccessRangeFromLastSeenLocation = 1000.0f;

		PerceptionComp->ConfigureSense(*SightConfig);
		PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
	}

	// 绑定感知事件
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AFPS_AI_Controller::OnTargetPerceived);
}

void AFPS_AI_Controller::OnTargetPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;
	if (!Stimulus.WasSuccessfullySensed())
	{
		if (LastPerceivedEnemy.Get() == Actor)
		{
			LastPerceivedEnemy = nullptr;
		}
		return;
	}

	// 检查 Actor 是不是敌人（不同队伍）
	APawn* SelfPawn = GetPawn();
	if (!SelfPawn) return;

	AFPS_PlayerState* SelfPS = GetPlayerState<AFPS_PlayerState>();
	AFPS_PlayerState* TargetPS = nullptr;

	if (AController* TargetController = Cast<AController>(Actor))
	{
		TargetPS = TargetController->GetPlayerState<AFPS_PlayerState>();
	}
	else if (APawn* TargetPawn = Cast<APawn>(Actor))
	{
		if (AController* TargetCtrl = TargetPawn->GetController())
		{
			TargetPS = TargetCtrl->GetPlayerState<AFPS_PlayerState>();
		}
	}

	// 不同队伍视为敌人
	if (SelfPS && TargetPS && SelfPS->PlayerTeam != TargetPS->PlayerTeam)
	{
		LastPerceivedEnemy = Actor;

		// 更新黑板
		UBlackboardComponent* BB = GetBlackboardComponent();
		if (BB)
		{
			BB->SetValueAsObject(TEXT("TargetActor"), Actor);
			BB->SetValueAsVector(TEXT("TargetLocation"), Actor->GetActorLocation());
			BB->SetValueAsBool(TEXT("bHasTarget"), true);
		}
	}
}

// ─── 寻找敌人 ──────────────────────────────────

AActor* AFPS_AI_Controller::GetNearestEnemy() const
{
	APawn* SelfPawn = GetPawn();
	if (!SelfPawn) return nullptr;

	AFPS_PlayerState* SelfPS = GetPlayerState<AFPS_PlayerState>();
	if (!SelfPS) return nullptr;

	AActor* NearestEnemy = nullptr;
	float MinDist = FLT_MAX;

	// 遍历所有玩家控制器找敌人
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->GetPawn()) continue;

		AFPS_PlayerState* TargetPS = PC->GetPlayerState<AFPS_PlayerState>();
		if (!TargetPS || TargetPS->PlayerTeam == SelfPS->PlayerTeam) continue;

		float Dist = FVector::Dist(SelfPawn->GetActorLocation(), PC->GetPawn()->GetActorLocation());
		if (Dist < MinDist)
		{
			MinDist = Dist;
			NearestEnemy = PC->GetPawn();
		}
	}

	// 也检查其他 AI
	TArray<AActor*> FoundAIs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFPS_AI_Controller::StaticClass(), FoundAIs);
	for (AActor* AIControllerActor : FoundAIs)
	{
		AFPS_AI_Controller* OtherAICtrl = Cast<AFPS_AI_Controller>(AIControllerActor);
		if (!OtherAICtrl || OtherAICtrl == this) continue;
		if (!OtherAICtrl->GetPawn()) continue;

		AFPS_PlayerState* TargetPS = OtherAICtrl->GetPlayerState<AFPS_PlayerState>();
		if (!TargetPS || TargetPS->PlayerTeam == SelfPS->PlayerTeam) continue;

		float Dist = FVector::Dist(SelfPawn->GetActorLocation(), OtherAICtrl->GetPawn()->GetActorLocation());
		if (Dist < MinDist)
		{
			MinDist = Dist;
			NearestEnemy = OtherAICtrl->GetPawn();
		}
	}

	return NearestEnemy;
}

// ─── 执行攻击 ──────────────────────────────────

void AFPS_AI_Controller::PerformAttack(AActor* Target)
{
	if (!HasAuthority() || !Target || !bCanAttack) return;
	if (!GetPawn()) return;

	APawn* TargetPawn = Cast<APawn>(Target);
	if (!TargetPawn) return;

	AController* TargetController = TargetPawn->GetController();
	if (!TargetController) return;

	// 造成伤害
	UGameplayStatics::ApplyDamage(
		TargetPawn,
		AttackDamage,
		this,
		GetPawn(),
		UDamageType::StaticClass()
	);

	// 进入冷却
	bCanAttack = false;
	GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimer, this, &AFPS_AI_Controller::OnAttackCooldownEnd, AttackCooldown, false);

	UE_LOG(LogTemp, Log, TEXT("[FPS_AI] Attacked %s for %.1f damage"), *Target->GetName(), AttackDamage);
}

void AFPS_AI_Controller::OnAttackCooldownEnd()
{
	bCanAttack = true;
}
