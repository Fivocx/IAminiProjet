#include "AttackLeaderTargetGoal_Evaluator.h"
#include "goals/Goal_Think.h"
#include "goals/Raven_Goal_Types.h"
#include "Raven_WeaponSystem.h"
#include "Raven_ObjectEnumerations.h"
#include "misc/Cgdi.h"
#include "misc/Stream_Utility_Functions.h"
#include "goals/Raven_Feature.h"


#include "Debug/DebugConsole.h"

//------------------ CalculateDesirability ------------------------------------
//
//  returns a value between 0 and 1 that indicates the Rating of a bot (the
//  higher the score, the stronger the bot).
//-----------------------------------------------------------------------------
double AttackLeaderTargetGoal_Evaluator::CalculateDesirability(Raven_Bot* pBot)
{
	double Desirability = 0.0;

	//only do the calculation if there is a target present
	if (!pBot->GetisLeader())
	{
		if (pBot->GetOwnLeader()->GetTargetLowLeader())
		{
			const double Tweaker = 1.0;

			Desirability = Tweaker * 10;

			//bias the value according to the personality of the bot
		}
	}
	

	return Desirability;
}

//----------------------------- SetGoal ---------------------------------------
//-----------------------------------------------------------------------------
void AttackLeaderTargetGoal_Evaluator::SetGoal(Raven_Bot* pBot)
{
	pBot->GetBrain()->AddGoal_AttackLeaderTarget();
}

//-------------------------- RenderInfo ---------------------------------------
//-----------------------------------------------------------------------------
void AttackLeaderTargetGoal_Evaluator::RenderInfo(Vector2D Position, Raven_Bot* pBot)
{
	gdi->TextAtPos(Position, "AT: " + ttos(CalculateDesirability(pBot), 2));
	return;

	std::string s = ttos(Raven_Feature::Health(pBot)) + ", " + ttos(Raven_Feature::TotalWeaponStrength(pBot));
	gdi->TextAtPos(Position + Vector2D(0, 12), s);
}