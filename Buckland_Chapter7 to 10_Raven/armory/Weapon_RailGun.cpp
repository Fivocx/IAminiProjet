#include "Weapon_RailGun.h"
#include "../Raven_Bot.h"
#include "../../Common/misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "../../Common/fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RailGun::RailGun(Raven_Bot*   owner):

                      Raven_Weapon(type_rail_gun,
                                   script->GetInt("RailGun_DefaultRounds"),
                                   script->GetInt("RailGun_MaxRoundsCarried"),
                                   script->GetDouble("RailGun_FiringFreq"),
                                   script->GetDouble("RailGun_IdealRange"),
                                   script->GetDouble("Slug_MaxSpeed"),
                                   owner)
{

    //setup the vertex buffer
  const int NumWeaponVerts = 4;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -1),
                                           Vector2D(10, -1),
                                           Vector2D(10, 1),
                                           Vector2D(0, 1)
                                           };

  
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------

inline void RailGun::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire a round
    m_pOwner->GetWorld()->AddRailGunSlug(m_pOwner, pos);

    UpdateTimeWeaponIsNextAvailable();

    m_iNumRoundsLeft--;

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RailGun_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RailGun::GetDesirability(double DistToTarget, double TangentialSpeed)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
	 m_FuzzyModule.Fuzzify("TangencialSpeed", min(TangentialSpeed, 100));
    m_FuzzyModule.Fuzzify("DistanceToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//----------------------- InitializeFuzzyModule -------------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RailGun::InitializeFuzzyModule()
{ 
	FuzzyVariable& TanSpeed = m_FuzzyModule.CreateFLV("TangencialSpeed");
	FzSet& TanS_Fast = TanSpeed.AddRightShoulderSet("Fast", 3.5, 5, 1000);
	FzSet& TanS_Medium = TanSpeed.AddTriangularSet("Medium", 1.5, 3, 4.5);
	FzSet& TanS_Slow = TanSpeed.AddLeftShoulderSet("Slow", 0, 1, 2.5);

  FuzzyVariable& DistanceToTarget = m_FuzzyModule.CreateFLV("DistanceToTarget");
  
  FzSet& Target_Close = DistanceToTarget.AddLeftShoulderSet("Target_Close", 0, 25, 150);
  FzSet& Target_Medium = DistanceToTarget.AddTriangularSet("Target_Medium", 25, 150, 300);
  FzSet& Target_Far = DistanceToTarget.AddRightShoulderSet("Target_Far", 150, 300, 1000);

  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
  
  FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 50, 75, 100);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 25, 50, 75);
  FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 25, 50);

  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
  FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 15, 30, 100);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 0, 15, 30);
  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 0, 15);

  

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Fast), FzFairly(Desirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Fast),  Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Fast), FzVery(Undesirable));

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Fast), FzFairly(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Fast), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Fast), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Fast), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Fast), FzFairly(Desirable));
  m_FuzzyModule.AddRule(FzAND(Target_Far, FzFairly(Ammo_Low), TanS_Fast), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Medium), FzFairly(Desirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Medium), FzFairly(Desirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Medium), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Medium), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Medium), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Medium), Desirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Medium), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Medium), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Far, FzFairly(Ammo_Low), TanS_Medium), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Slow), FzFairly(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Slow), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Slow), Desirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Slow), FzFairly(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Slow), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Slow), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Slow), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Slow), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Far, FzFairly(Ammo_Low), TanS_Slow), FzVery(VeryDesirable));

}

//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RailGun::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->BluePen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}