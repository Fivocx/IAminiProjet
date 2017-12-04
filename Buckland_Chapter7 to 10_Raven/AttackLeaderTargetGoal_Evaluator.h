#ifndef RAVEN_ATTACK_LEADER_GOAL_EVALUATOR
#define RAVEN_ATTACK_LEADER_GOAL_EVALUATOR
#pragma warning (disable:4786)

#include "../Buckland_Chapter7 to 10_Raven/Raven_Bot.h"
#include "goals\Goal_Evaluator.h"


class AttackLeaderTargetGoal_Evaluator : public Goal_Evaluator
{
public:

	AttackLeaderTargetGoal_Evaluator(double bias) :Goal_Evaluator(bias) {}

	double CalculateDesirability(Raven_Bot* pBot);

	void  SetGoal(Raven_Bot* pEnt);

	void RenderInfo(Vector2D Position, Raven_Bot* pBot);
};



#endif
