#include "Foundation/Characters/ARBaseEnemy.h"

#include "Foundation/AI/ARAIController.h"

AARBaseEnemy::AARBaseEnemy()
{
	CombatTeam = EARCombatTeam::Enemy;
	AIControllerClass = AARAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

