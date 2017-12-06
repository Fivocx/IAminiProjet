#include "Projectile_HealingRay.h"


#include "lua/Raven_Scriptor.h"
#include "misc/cgdi.h"
#include "Raven_Bot.h"
#include "Raven_Game.h"
#include "game/EntityFunctionTemplates.h"
#include "2d/WallIntersectionTests.h"
#include "Raven_Map.h"

#include "Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"

#include <list>



HealingRay::HealingRay(Raven_Bot* shooter, Vector2D target) :

	Raven_Projectile(target,
		shooter->GetWorld(),
		shooter->ID(),
		shooter->Pos(),
		shooter->Facing(),
		script->GetInt("HealingRay_HealthGiven"),
		script->GetDouble("HealingRay_Scale"),
		script->GetDouble("HealingRay_MaxSpeed"),
		script->GetDouble("HealingRay_Mass"),
		script->GetDouble("HealingRay_MaxForce")),

	m_dTimeShotIsVisible(script->GetDouble("HealingRay_Persistance"))
{

}


void HealingRay::Update()
{
	if (!HasImpacted())
	{
		//calculate the steering force
		Vector2D DesiredVelocity = Vec2DNormalize(m_vTarget - Pos()) * MaxSpeed();

		Vector2D sf = DesiredVelocity - Velocity();

		//update the position
		Vector2D accel = sf / m_dMass;

		m_vVelocity += accel;

		//make sure the slug does not exceed maximum velocity
		m_vVelocity.Truncate(m_dMaxSpeed);

		//update the position
		m_vPosition += m_vVelocity;

		TestForImpact();
	}
	else if (!isVisibleToPlayer())
	{
		m_bDead = true;
	}

	MovingEntity::Update();
}

//----------------------------------- TestForImpact ---------------------------
void HealingRay::TestForImpact()
{
	// a rail gun slug travels VERY fast. It only gets the chance to update once 
	m_bImpacted = true;

	//first find the closest wall that this ray intersects with. Then we
	//can test against all entities within this range.
	double DistToClosestImpact;
	FindClosestPointOfIntersectionWithWalls(m_vOrigin,
		m_vPosition,
		DistToClosestImpact,
		m_vImpactPoint,
		m_pWorld->GetMap()->GetWalls());

	//test to see if the ray between the current position of the slug and 
	//the start position intersects with any bots.
	std::list<Raven_Bot*> hits = GetListOfIntersectingBots(m_vOrigin, m_vPosition);

	//if no bots hit just return;
	if (hits.empty()) return;

	//give some damage to the hit bots
	std::list<Raven_Bot*>::iterator it;
	for (it = hits.begin(); it != hits.end(); ++it)
	{
		(*it)->IncreaseHealth(m_iDamageInflicted);

		//send a message to the bot to let it know it's been hit, and who the
		//shot came from
		/*Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
			m_iShooterID,
			(*it)->ID(),
			Msg_TakeThatMF,
			(void*)&m_iDamageInflicted);*/

	}
}

//-------------------------- Render -------------------------------------------
//-----------------------------------------------------------------------------
void HealingRay::Render()
{
	if (isVisibleToPlayer() && m_bImpacted)
	{
		gdi->BluePen();
		gdi->Line(m_vOrigin, m_vImpactPoint);
	}
}   