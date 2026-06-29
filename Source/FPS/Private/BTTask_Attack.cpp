// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_Attack.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack Target");
	Damage = 20.0f;
	RequiredRange = 2000.0f;
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	APawn* TargetPawn = Cast<APawn>(Target);
	if (!TargetPawn)
	{
		return EBTNodeResult::Failed;
	}

	// 检查距离
	float Distance = FVector::Dist(AIController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
	if (Distance > RequiredRange * 1.2f)
	{
		// 太远，让行为树重新寻路
		return EBTNodeResult::Failed;
	}

	// 造成伤害（仅在服务器上）
	if (AIController->HasAuthority())
	{
		UGameplayStatics::ApplyDamage(
			TargetPawn,
			Damage,
			AIController,
			AIController->GetPawn(),
			UDamageType::StaticClass()
		);
	}

	return EBTNodeResult::Succeeded;
}
