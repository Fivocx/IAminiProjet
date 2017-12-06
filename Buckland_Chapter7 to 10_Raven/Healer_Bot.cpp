#include "Healer_Bot.h"
#include "Raven_Bot.h"
#include "misc/Cgdi.h"
#include "misc/utils.h"
#include "2D/Transformations.h"
#include "2D/Geometry.h"
#include "lua/Raven_Scriptor.h"
#include "Raven_Game.h"
#include "navigation/Raven_PathPlanner.h"
#include "Raven_SteeringBehaviors.h"
#include "Raven_UserOptions.h"
#include "time/Regulator.h"
#include "Raven_WeaponSystem.h"
#include "Raven_SensoryMemory.h"

#include "Messaging/Telegram.h"
#include "Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"

#include "goals/Raven_Goal_Types.h"
#include "goals/Goal_Think.h"


#include "Debug/DebugConsole.h"



Healer_Bot::Healer_Bot(Raven_Game* world, Vector2D pos, bool isleader, bool autreEquipe, Raven_Bot* leaderPt)
	:Raven_Bot(world, pos, isleader, autreEquipe, leaderPt)
{

}


Healer_Bot::~Healer_Bot()
{
}



void Healer_Bot::Update()
{

	MovingEntity::Update();
	//process the currently active goal. Note this is required even if the bot
	//is under user control. This is because a goal is created whenever a user 
	//clicks on an area of the map that necessitates a path planning request.
	m_pBrain->Process();

	//Calculate the steering force and update the bot's velocity and position
	UpdateMovement();

	//if the bot is under AI control but not scripted
	if (!isPossessed())
	{
		//examine all the opponents in the bots sensory memory and select one
		//to be the current target
		if (m_pTargetSelectionRegulator->isReady())
		{
			m_pTargSys->GetAlliedTarget();
		}

		//appraise and arbitrate between all possible high level goals
		if (m_pGoalArbitrationRegulator->isReady())
		{
			GetBrain()->Arbitrate();
		}

		//update the sensory memory with any visual stimulus
		if (m_pVisionUpdateRegulator->isReady())
		{
			GetSensoryMem()->UpdateVision();
		}

		//select the appropriate weapon to use from the weapons currently in
		//the inventory
		if (m_pWeaponSelectionRegulator->isReady())
		{
			GetWeaponSys()->SelectWeapon();
		}

		//this method aims the bot's current weapon at the current target
		//and takes a shot if a shot is possible
		m_pWeaponSys->SetAccuracy();
		m_pWeaponSys->TakeAimAndShoot();
	}

}


void Healer_Bot::Render()
{
	//when a bot is hit by a projectile this value is set to a constant user
	//defined value which dictates how long the bot should have a thick red
	//circle drawn around it (to indicate it's been hit) The circle is drawn
	//as long as this value is positive. (see Render)
	m_iNumUpdatesHitPersistant--;


	if (isDead() || isSpawning()) return;

	gdi->BluePen();

	m_vecBotVBTrans = WorldTransform(m_vecBotVB,
		Pos(),
		Facing(),
		Facing().Perp(),
		Scale());

	gdi->ClosedShape(m_vecBotVBTrans);

	//draw the head
	if (!GetisEquipe2())
	{
		gdi->RedBrush();
	}
	else
	{
		gdi->BlueBrush();
	}

	gdi->Circle(Pos(), 6.0 * Scale().x);


	//render the bot's weapon
	m_pWeaponSys->RenderCurrentWeapon();

	//render a thick red circle if the bot gets hit by a weapon
	if (m_bHit)
	{
		gdi->ThickRedPen();
		gdi->HollowBrush();
		gdi->Circle(m_vPosition, BRadius() + 1);

		if (m_iNumUpdatesHitPersistant <= 0)
		{
			m_bHit = false;
		}
	}

	gdi->TransparentText();
	gdi->TextColor(0, 255, 0);

	if (UserOptions->m_bShowBotIDs)
	{
		gdi->TextAtPos(Pos().x - 10, Pos().y - 20, std::to_string(ID()));
	}

	if (UserOptions->m_bShowBotHealth)
	{
		gdi->TextAtPos(Pos().x - 40, Pos().y - 5, "H:" + std::to_string(Health()));
	}

	if (UserOptions->m_bShowScore)
	{
		gdi->TextAtPos(Pos().x - 40, Pos().y + 10, "Scr:" + std::to_string(Score()));
	}
	gdi->TextColor(0, 0, 255);
	if (isLeader)
	{
		gdi->TextAtPos(Pos().x - 10, Pos().y - 20, "Healer");
	}

}	