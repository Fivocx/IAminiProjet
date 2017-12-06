#include "Raven_TargetingSystem.h"
#include "Raven_Bot.h"
#include "Raven_SensoryMemory.h"



//-------------------------------- ctor ---------------------------------------
//-----------------------------------------------------------------------------
Raven_TargetingSystem::Raven_TargetingSystem(Raven_Bot* owner):m_pOwner(owner),
                                                               m_pCurrentTarget(0)
{}



//----------------------------- Update ----------------------------------------

//-----------------------------------------------------------------------------
void Raven_TargetingSystem::GetEnemyTarget()
{
  double ClosestDistSoFar = MaxDouble;
  m_pCurrentTarget       = 0;

  //grab a list of all the opponents the owner can sense
  std::list<Raven_Bot*> EnemyBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedOpponents();
  std::list<Raven_Bot*> AllyBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedAllies();

  std::list<Raven_Bot*>::const_iterator curBot = AllyBots.begin();
  Vector2D teamMiddle = m_pOwner->Pos();
  int count = 1;
  for (curBot; curBot != AllyBots.end(); ++curBot)
  {
	  teamMiddle += (*curBot)->Pos();
	  count++;
  }
  teamMiddle = teamMiddle / count;

  
  /*
  if ((!m_pOwner->GetisLeader()) && (m_pOwner->GetOwnLeader()->GetTargetLowLeader()))
  {
	  m_pCurrentTarget = m_pOwner->GetOwnLeader()->GetTargetLowLeader();
  }
  else */

  const double selfDistanceWeigth = 0.33;
  const double teamDistanceWeigth = 0.66;

  curBot = EnemyBots.begin();
	for (curBot; curBot != EnemyBots.end(); ++curBot)
	{
		if ( ( *curBot)->isAlive()) 
		{
			double distSelf = Vec2DDistanceSq((*curBot)->Pos(), m_pOwner->Pos());
			double distTeam= Vec2DDistanceSq((*curBot)->Pos(), teamMiddle);
			double distWeigth = (distSelf * selfDistanceWeigth + distTeam * teamDistanceWeigth);

			if ((m_pOwner->GetisLeader()) && ((*curBot)->Health() <= 50))
			{
				m_pCurrentTarget = *curBot;
				m_pOwner->SetTargetLowLeader(*curBot);
			}

			else if (distWeigth < ClosestDistSoFar)
			{
				ClosestDistSoFar = distWeigth;
				m_pCurrentTarget = *curBot;
				m_pOwner->SetTargetLowLeader(nullptr);
			}
		}
	}
  

}

void Raven_TargetingSystem::GetAlliedTarget()
{
	double LowestHp = 10000000;
	m_pCurrentTarget = 0;

	//grab a list of all the opponents the owner can sense
	std::list<Raven_Bot*> SensedBots;
	SensedBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedAllies();

	std::list<Raven_Bot*>::const_iterator curBot = SensedBots.begin();

	for (curBot; curBot != SensedBots.end(); ++curBot)
	{
		//make sure the bot is alive and that it is not the owner
		if ((*curBot)->isAlive() && (*curBot != m_pOwner))// &&  (*curBot)->SameTeam(m_pOwner))
		{
			double health = (*curBot)->Health();


			if (health < LowestHp && health != (*curBot)->MaxHealth())
			{
				LowestHp = health;
				m_pCurrentTarget = *curBot;
				m_pOwner->SetTargetLowLeader(nullptr);
			}
		}
	}
}


bool Raven_TargetingSystem::isTargetWithinFOV()const
{
  return m_pOwner->GetSensoryMem()->isOpponentWithinFOV(m_pCurrentTarget);
}

bool Raven_TargetingSystem::isTargetShootable()const
{
  return m_pOwner->GetSensoryMem()->isOpponentShootable(m_pCurrentTarget);
}

Vector2D Raven_TargetingSystem::GetLastRecordedPosition()const
{
  return m_pOwner->GetSensoryMem()->GetLastRecordedPositionOfOpponent(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenVisible()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenVisible(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenOutOfView()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenOutOfView(m_pCurrentTarget);
}
