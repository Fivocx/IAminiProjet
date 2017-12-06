#ifndef GOAL_ATTACKLEADERTARGET_H
#define GOAL_ATTACKLEADERTARGET_H
#pragma warning (disable:4786)

#include "Goals/Goal_Composite.h"
#include "goals/Raven_Goal_Types.h"
#include "../Buckland_Chapter7 to 10_Raven/Raven_Bot.h"





class Goal_AttackLeaderTarget : public Goal_Composite<Raven_Bot>
{
public:

	Goal_AttackLeaderTarget(Raven_Bot* pOwner) :Goal_Composite<Raven_Bot>(pOwner, goal_attack_leader_target)
	{}

	void Activate();

	int  Process();

	void Terminate() { m_iStatus = completed; }

};






#endif

