#include "Weapon_RocketLauncher.h"
#include "../Raven_Bot.h"
#include "../../Common/misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "../../Common/fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RocketLauncher::RocketLauncher(Raven_Bot*   owner):
					  blastRadius(script->GetDouble("Rocket_BlastRadius")),
                      Raven_Weapon(type_rocket_launcher,
                                   script->GetInt("RocketLauncher_DefaultRounds"),
                                   script->GetInt("RocketLauncher_MaxRoundsCarried"),
                                   script->GetDouble("RocketLauncher_FiringFreq"),
                                   script->GetDouble("RocketLauncher_IdealRange"),
                                   script->GetDouble("Rocket_MaxSpeed"),
                                   owner)
{
    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -3),
                                           Vector2D(6, -3),
                                           Vector2D(6, -1),
                                           Vector2D(15, -1),
                                           Vector2D(15, 1),
                                           Vector2D(6, 1),
                                           Vector2D(6, 3),
                                           Vector2D(0, 3)
                                           };
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }
  

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void RocketLauncher::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire off a rocket!
    m_pOwner->GetWorld()->AddRocket(m_pOwner, pos);

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RocketLauncher_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RocketLauncher::GetDesirability(double DistToTarget, double PerpendicularSpeed)
{
	std::list<Raven_Bot*>::const_iterator curBot = m_pOwner->GetWorld()->GetAllBots().begin();
	int enemiesInRange = 0;
	int alliesInRange = 0;
	for (curBot; curBot != m_pOwner->GetWorld()->GetAllBots().end(); ++curBot)
	{
		if (Vec2DDistance(m_pOwner->Pos(), (*curBot)->Pos()) < blastRadius + (*curBot)->BRadius())
		{
			if ((*curBot)->GetisEquipe2() != m_pOwner->GetisEquipe2())
				enemiesInRange++;
			else
				alliesInRange++;
		}
	}
	int enemiesDifferencial = enemiesInRange - alliesInRange;
	
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
	m_FuzzyModule.Fuzzify("EnemiesInRange", enemiesDifferencial);
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RocketLauncher::InitializeFuzzyModule()
{
	FuzzyVariable& EnemiesInRange = m_FuzzyModule.CreateFLV("EnemiesInRange");
	FzSet& Enemies_Bad = EnemiesInRange.AddLeftShoulderSet("Terrible", -100, -1, 1);
	FzSet& Enemies_Good = EnemiesInRange.AddRightShoulderSet("Great", 0, 2, 100);

	FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");
	FzSet& Target_Close = DistToTarget.AddLeftShoulderSet("Target_Close", 0, 25, 150);
	FzSet& Target_MedClose = DistToTarget.AddTriangularSet("Target_MedClose", 25, 150, 300);
	FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium", 150, 300, 450);
	FzSet& Target_MedFar = DistToTarget.AddTriangularSet("Target_MedFar", 300, 450, 575);
	FzSet& Target_Far = DistToTarget.AddRightShoulderSet("Target_Far", 450, 575, 1000);

	FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
	FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 65, 80, 100);
	FzSet& QuiteDesirable = Desirability.AddTriangularSet("QuiteDesirable", 50, 65, 80);
	FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 35, 50, 65);
	FzSet& BarelyDesirable = Desirability.AddTriangularSet("BarelyDesirable", 20, 35, 50);
	FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 20, 35);

	FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
	FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 40, 60, 100);
	FzSet& Ammo_High = AmmoStatus.AddTriangularSet("Ammo_High", 20, 40, 60);
	FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 20, 40);
	FzSet& Ammo_Barely = AmmoStatus.AddTriangularSet("Ammo_Barely", 0, 10, 20);
	FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 0, 10);


	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Barely, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, Enemies_Good), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Loads, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_High, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Okay, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Barely, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Low, Enemies_Good), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, Enemies_Good), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Barely, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, Enemies_Good), BarelyDesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Loads, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_High, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Okay, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Barely, Enemies_Good), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Low, Enemies_Good), Desirable);

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Barely, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, Enemies_Good), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Barely, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, Enemies_Good), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Loads, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_High, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Okay, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Barely, Enemies_Good), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Low), Enemies_Good);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, Enemies_Good), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Barely, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, Enemies_Good), BarelyDesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Loads, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_High, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Okay, Enemies_Good), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Barely, Enemies_Good), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Low, Enemies_Good), Desirable);

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High, Enemies_Good), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Barely, Enemies_Good), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, Enemies_Good), Undesirable);






	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Low), Undesirable), FzVery(Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Low, Enemies_Bad), FzVery(Undesirable));

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Barely, Enemies_Bad), FzVery(Undesirable));
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, Enemies_Bad), FzVery(Undesirable));
}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RocketLauncher::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->RedPen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}