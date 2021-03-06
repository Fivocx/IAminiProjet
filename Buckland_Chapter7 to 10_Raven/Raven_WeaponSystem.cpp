#include "Raven_WeaponSystem.h"
#include "armory/Weapon_RocketLauncher.h"
#include "armory/Weapon_RailGun.h"
#include "armory/Weapon_ShotGun.h"
#include "armory/Weapon_Blaster.h"
#include "armory/Weapon_Grenade.h"
#include "Weapon_HealingRayGun.h"
#include "Raven_Bot.h"
#include "misc/utils.h"
#include "lua/Raven_Scriptor.h"
#include "Raven_Game.h"
#include "Raven_UserOptions.h"
#include "2D/transformations.h"

#include "fuzzy/FuzzyOperators.h"
#include <cmath>
#include <vector>


//------------------------- ctor ----------------------------------------------
//-----------------------------------------------------------------------------
Raven_WeaponSystem::Raven_WeaponSystem(Raven_Bot* owner,
                                       double ReactionTime,
                                       double AimAccuracy,
                                       double AimPersistance,
									   unsigned int avalaibleWeapons):m_pOwner(owner),
                                                          m_dReactionTime(ReactionTime),
                                                          m_dAimAccuracy(AimAccuracy),
                                                          m_dAimPersistance(AimPersistance)
{
  Initialize(avalaibleWeapons);
}

//------------------------- dtor ----------------------------------------------
//-----------------------------------------------------------------------------
Raven_WeaponSystem::~Raven_WeaponSystem()
{
  for (unsigned int w=0; w<m_WeaponMap.size(); ++w)
  {
    delete m_WeaponMap[w];
  }
}

//------------------------------ Initialize -----------------------------------
//
//  initializes the weapons
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::Initialize(unsigned int defaultWeapon)
{
	InitializeFuzzyModule();
  //delete any existing weapons
  WeaponMap::iterator curW;
  for (curW = m_WeaponMap.begin(); curW != m_WeaponMap.end(); ++curW)
  {
    delete curW->second;
  }

  this->defaultWeapon = defaultWeapon;
  m_WeaponMap.clear();

  //set up the container

	  switch (defaultWeapon)
	  {
	  case type_blaster:
		  m_pCurrentWeapon = new Blaster(m_pOwner);
		  m_WeaponMap[type_blaster] = m_pCurrentWeapon;
		  break;
	  case type_shotgun:
		  m_pCurrentWeapon = new ShotGun(m_pOwner);
		  m_WeaponMap[type_shotgun] = m_pCurrentWeapon;
		  break;
	  case type_rail_gun:
		  m_pCurrentWeapon = new RailGun(m_pOwner);
		  m_WeaponMap[type_rail_gun] = m_pCurrentWeapon;
		  break;
	  case type_rocket_launcher:
		  m_pCurrentWeapon = new RocketLauncher(m_pOwner);
		  m_WeaponMap[type_rocket_launcher] = m_pCurrentWeapon;
		  break;
	  case type_HealingRayGun:
		  m_pCurrentWeapon = new HealingRayGun(m_pOwner);
		  m_WeaponMap[type_HealingRayGun] = m_pCurrentWeapon;
		  break;
	  case type_grenade:
		  m_pCurrentWeapon = new Weapon_Grenade(m_pOwner);
		  m_WeaponMap[type_grenade] = m_pCurrentWeapon;
		  break;
	  default:
		  break;
	  }

  
  if (defaultWeapon != type_blaster)
	  m_WeaponMap[type_blaster] = 0;
  if (defaultWeapon != type_shotgun)
	  m_WeaponMap[type_shotgun] = 0;
  if (defaultWeapon != type_rail_gun)
	  m_WeaponMap[type_rail_gun] = 0;
  if (defaultWeapon != type_rocket_launcher)
	  m_WeaponMap[type_rocket_launcher] = 0;
  if (defaultWeapon != type_grenade)
	  m_WeaponMap[type_grenade] = 0;
  if (defaultWeapon != type_HealingRayGun)
	  m_WeaponMap[type_HealingRayGun] = 0;



}
void Raven_WeaponSystem::Reset()
{
		m_WeaponMap[type_blaster] = 0;
		m_WeaponMap[type_shotgun] = 0;
		m_WeaponMap[type_rail_gun] = 0;
		m_WeaponMap[type_rocket_launcher] = 0;
		m_WeaponMap[type_grenade] = 0;
		m_WeaponMap[type_HealingRayGun] = 0;
}
//-------------------------------- SelectWeapon -------------------------------
//
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::SelectWeapon()
{ 
  //if a target is present use fuzzy logic to determine the most desirable 
  //weapon.
  if (m_pOwner->GetTargetSys()->isTargetPresent())
  {
    //calculate the distance to the target

	double TangentialSpeed = m_pOwner->RelativeTangencialSpeed(m_pOwner->GetTargetSys()->GetTarget());
	double DistToTarget = (m_pOwner->Pos() - m_pOwner->GetTargetSys()->GetTarget()->GetPreviousState(m_dReactionTime).Pos()).Length();

    //for each weapon in the inventory calculate its desirability given the 
    //current situation. The most desirable weapon is selected
    double BestSoFar = MinDouble;

    WeaponMap::const_iterator curWeap;
    for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
    {
      //grab the desirability of this weapon (desirability is based upon
      //distance to target and ammo remaining)
      if (curWeap->second)
      {
		  TangentialSpeed = max(TangentialSpeed, 100);
		  double score = curWeap->second->GetDesirability(DistToTarget, TangentialSpeed);

        //if it is the most desirable so far select it
        if (score > BestSoFar)
        {
          BestSoFar = score;

          //place the weapon in the bot's hand.
          m_pCurrentWeapon = curWeap->second;
        }
      }
    }
  }

  else
  {
    m_pCurrentWeapon = m_WeaponMap[defaultWeapon];
  }
}

//--------------------  AddWeapon ------------------------------------------
//
//  this is called by a weapon affector and will add a weapon of the specified
//  type to the bot's inventory.
//
//  if the bot already has a weapon of this type then only the ammo is added
//-----------------------------------------------------------------------------
void  Raven_WeaponSystem::AddWeapon(unsigned int weapon_type)
{
  //create an instance of this weapon
  Raven_Weapon* w = 0;

  switch(weapon_type)
  {
  case type_rail_gun:

    w = new RailGun(m_pOwner); break;

  case type_shotgun:

    w = new ShotGun(m_pOwner); break;

  case type_rocket_launcher:

    w = new RocketLauncher(m_pOwner); break;

  case type_grenade:

	  w = new Weapon_Grenade(m_pOwner); break;

  }//end switch
  

  //if the bot already holds a weapon of this type, just add its ammo
  Raven_Weapon* present = GetWeaponFromInventory(weapon_type);

  if (present)
  {
    present->IncrementRounds(w->NumRoundsRemaining());

    delete w;
  }
  
  //if not already holding, add to inventory
  else
  {
    m_WeaponMap[weapon_type] = w;
  }
}

void  Raven_WeaponSystem::AddWeapon(Raven_Weapon* weapon)
{
	Raven_Weapon* present = GetWeaponFromInventory(weapon->GetType());

	if (present)
	{
		present->IncrementRounds(weapon->NumRoundsRemaining());
		delete weapon;
	}
	else
	{
		m_WeaponMap[weapon->GetType()] = weapon;
	}
}

//------------------------- GetWeaponFromInventory -------------------------------
//
//  returns a pointer to any matching weapon.
//
//  returns a null pointer if the weapon is not present
//-----------------------------------------------------------------------------
Raven_Weapon* Raven_WeaponSystem::GetWeaponFromInventory(int weapon_type)
{
  return m_WeaponMap[weapon_type];
}

//----------------------- ChangeWeapon ----------------------------------------
void Raven_WeaponSystem::ChangeWeapon(unsigned int type)
{
  Raven_Weapon* w = GetWeaponFromInventory(type);

  if (w) m_pCurrentWeapon = w;
}

//--------------------------- TakeAimAndShoot ---------------------------------
//
//  this method aims the bots current weapon at the target (if there is a
//  target) and, if aimed correctly, fires a round
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::TakeAimAndShoot()const
{
  //aim the weapon only if the current target is shootable or if it has only
  //very recently gone out of view (this latter condition is to ensure the 
  //weapon is aimed at the target even if it temporarily dodges behind a wall
  //or other cover)


  if (m_pOwner->GetTargetSys()->isTargetShootable() ||
      (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenOutOfView() < 
       m_dAimPersistance) )
  {
		//the position the weapon will be aimed at
	  Vector2D AimingPos;
	  if(GetCurrentWeapon()->GetType() == type_HealingRayGun)
		   AimingPos = m_pOwner->GetTargetBot()->Pos();
	  else
		   AimingPos = m_pOwner->GetTargetBot()->GetPreviousState(m_dReactionTime).Pos();

		//if the current weapon is not an instant hit type gun the target position
		//must be adjusted to take into account the predicted movement of the 
		//target
		if (GetCurrentWeapon()->GetType() == type_rocket_launcher ||
			GetCurrentWeapon()->GetType() == type_blaster)
			{
			  AimingPos = PredictFuturePositionOfTarget();

			  //if the weapon is aimed correctly, there is line of sight between the
			  //bot and the aiming position and it has been in view for a period longer
			  //than the bot's reaction time, shoot the weapon
			  if ( m_pOwner->RotateFacingTowardPosition(AimingPos) &&
				   (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenVisible() >
					m_dReactionTime) &&
				   m_pOwner->hasLOSto(AimingPos) )
				  {

				  if (GetCurrentWeapon()->GetType() != type_HealingRayGun)
					  AddNoiseToAim(AimingPos);
					

					GetCurrentWeapon()->ShootAt(AimingPos);
				  }
		}

		//no need to predict movement, aim directly at target
		else
		{
			  //if the weapon is aimed correctly and it has been in view for a period
			  //longer than the bot's reaction time, shoot the weapon
			  if ( m_pOwner->RotateFacingTowardPosition(AimingPos) &&
				   (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenVisible() >
					m_dReactionTime) )
			  {
				AddNoiseToAim(AimingPos);
        
				GetCurrentWeapon()->ShootAt(AimingPos);
			  }
		 }

  }
  
  //no target to shoot at so rotate facing to be parallel with the bot's
  //heading direction
  else
  {
    m_pOwner->RotateFacingTowardPosition(m_pOwner->Pos()+ m_pOwner->Heading());
  }
}

//---------------------------- AddNoiseToAim ----------------------------------
//
//  adds a random deviation to the firing angle not greater than m_dAimAccuracy 
//  rads
//-----------------------------------------------------------------------------

void Raven_WeaponSystem::AddNoiseToAim(Vector2D& AimingPos)const
{
	Vector2D toPos = AimingPos - m_pOwner->Pos();
	
	// le m_dAimAccuracy est fixe avec un comportement flou, voir plus bas
	Vec2DRotateAroundOrigin(toPos, RandInRange(-m_dAimAccuracy, m_dAimAccuracy)) ;
	
	AimingPos = toPos + m_pOwner->Pos();
}

//-------------------------- PredictFuturePositionOfTarget --------------------
//
//  predicts where the target will be located in the time it takes for a
//  projectile to reach it. This uses a similar logic to the Pursuit steering
//  behavior.
//-----------------------------------------------------------------------------
Vector2D Raven_WeaponSystem::PredictFuturePositionOfTarget()const
{
  double MaxSpeed = GetCurrentWeapon()->GetMaxProjectileSpeed();
  
  //if the target is ahead and facing the agent shoot at its current pos
  Vector2D ToEnemy = m_pOwner->GetTargetBot()->Pos() - m_pOwner->Pos();
 
  //the lookahead time is proportional to the distance between the enemy
  //and the pursuer; and is inversely proportional to the sum of the
  //agent's velocities
  double LookAheadTime = ToEnemy.Length() / 
                        (MaxSpeed + m_pOwner->GetTargetBot()->MaxSpeed());
  
  //return the predicted future position of the enemy
  return m_pOwner->GetTargetBot()->Pos() + 
         m_pOwner->GetTargetBot()->Velocity() * LookAheadTime;
}


//------------------ GetAmmoRemainingForWeapon --------------------------------
//
//  returns the amount of ammo remaining for the specified weapon. Return zero
//  if the weapon is not present
//-----------------------------------------------------------------------------
int Raven_WeaponSystem::GetAmmoRemainingForWeapon(unsigned int weapon_type)
{
  if (m_WeaponMap[weapon_type])
  {
    return m_WeaponMap[weapon_type]->NumRoundsRemaining();
  }

  return 0;
}

//---------------------------- ShootAt ----------------------------------------
//
//  shoots the current weapon at the given position
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::ShootAt(Vector2D pos)const
{
  m_pOwner->AddShotFire();
  GetCurrentWeapon()->ShootAt(pos);
}

//-------------------------- RenderCurrentWeapon ------------------------------
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::RenderCurrentWeapon()const
{
  GetCurrentWeapon()->Render();
}

void Raven_WeaponSystem::RenderDesirabilities()const
{
  Vector2D p = m_pOwner->Pos();

  int num = 0;
  
  WeaponMap::const_iterator curWeap;
  for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
  {
    if (curWeap->second) num++;
  }

  int offset = 15 * num;

    for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
    {
      if (curWeap->second)
      {
        double score = curWeap->second->GetLastDesirabilityScore();
        std::string type = GetNameOfType(curWeap->second->GetType());

        gdi->TextAtPos(p.x+10.0, p.y-offset, std::to_string(score) + " " + type);

        offset+=15;
      }
    }
}



double Raven_WeaponSystem::SetAccuracy()
{

	double shotsFiredRecently =  m_pOwner->GetAmountOfShotFiredSince(RecoilTimeSpawn);
	double TangencialSpeed = (m_pOwner->GetTargetSys()->GetTarget() != nullptr ? m_pOwner->RelativeTangencialSpeed(m_pOwner->GetTargetBot()) : 0);

	m_FuzzyModule.Fuzzify("TangencialSpeed", min(TangencialSpeed, 100));
	m_FuzzyModule.Fuzzify("ShotsFiredRecently", shotsFiredRecently);

	m_dAimAccuracy = m_FuzzyModule.DeFuzzify("Accuracy", FuzzyModule::max_av);
	return m_dAimAccuracy;
}


void Raven_WeaponSystem::InitializeFuzzyModule()
{
	FuzzyVariable& TanSpeed = m_FuzzyModule.CreateFLV("TangencialSpeed");
	FzSet& TanS_Fast = TanSpeed.AddRightShoulderSet("Fast", 3.5, 5, 100);
	FzSet& TanS_Medium = TanSpeed.AddTriangularSet("Medium", 1.5, 3, 4.5);
	FzSet& TanS_Slow = TanSpeed.AddLeftShoulderSet("Slow", 0, 1, 2.5);

	FuzzyVariable& ShotsFired = m_FuzzyModule.CreateFLV("ShotsFiredRecently");
	FzSet& sFired_ALot = ShotsFired.AddLeftShoulderSet("A_Lot_of_shots", 5, 8, 64);
	FzSet& sFired_Acouple = ShotsFired.AddTriangularSet("A_Couple_of_shots", 2, 4, 7);
	FzSet& sFired_Few = ShotsFired.AddRightShoulderSet("Few_of_shots", 0, 1, 3);

	FuzzyVariable& Accuracy = m_FuzzyModule.CreateFLV("Accuracy");
	FzSet& TopGun = Accuracy.AddLeftShoulderSet("TopGun", 0, 0.25, 0.5);
	FzSet& OK = Accuracy.AddTriangularSet("OK", 0.25, 0.50, 0.125);
	FzSet& Noob = Accuracy.AddRightShoulderSet("Noob", 0.75, 0.15, 0.25);

	m_FuzzyModule.AddRule(FzAND(sFired_ALot, TanS_Fast), FzVery(Noob));
	m_FuzzyModule.AddRule(FzAND(sFired_Acouple, TanS_Fast), Noob);
	m_FuzzyModule.AddRule(FzAND(sFired_Few, TanS_Fast), OK);

	m_FuzzyModule.AddRule(FzAND(sFired_ALot, TanS_Medium), Noob);
	m_FuzzyModule.AddRule(FzAND(sFired_Acouple, TanS_Medium), OK);
	m_FuzzyModule.AddRule(FzAND(sFired_Few, TanS_Medium), TopGun);

	m_FuzzyModule.AddRule(FzAND(sFired_ALot, TanS_Slow), OK);
	m_FuzzyModule.AddRule(FzAND(sFired_Acouple, TanS_Slow), TopGun);
	m_FuzzyModule.AddRule(FzAND(sFired_Few, TanS_Slow), FzVery(TopGun));
}
