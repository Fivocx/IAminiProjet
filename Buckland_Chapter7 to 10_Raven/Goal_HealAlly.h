#pragma once

#include "../../Common/Goals/Goal_Composite.h"
#include "Goals\Raven_Goal_Types.h"
#include "Raven_Bot.h"


/*

class Goal_HealAlly : public Goal_Composite<Raven_Bot>
{
public: 

	Goal_HealAlly(Raven_Bot* pOwner) :Goal_Composite<Raven_Bot>(pOwner, goal_heal_ally)
	{}

	void Activate();

	int  Process();

	void Terminate() { m_iStatus = completed; }

};



*/