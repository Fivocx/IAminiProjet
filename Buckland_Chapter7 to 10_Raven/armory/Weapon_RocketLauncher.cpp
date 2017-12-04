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
double RocketLauncher::GetDesirability(double DistToTarget)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
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


	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_High), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Barely), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Loads), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_High), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Okay), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Barely), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedClose, Ammo_Low), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_High), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Barely), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), BarelyDesirable);

	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Loads), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_High), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Okay), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Barely), QuiteDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_MedFar, Ammo_Low), Desirable);

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_High), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Barely), BarelyDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
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