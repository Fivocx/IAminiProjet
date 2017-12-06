#include "Weapon_ShotGun.h"
#include "../Raven_Bot.h"
#include "../../Common/misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "../../Common/misc/utils.h"
#include "../../Common/fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
ShotGun::ShotGun(Raven_Bot*   owner):

                      Raven_Weapon(type_shotgun,
                                   script->GetInt("ShotGun_DefaultRounds"),
                                   script->GetInt("ShotGun_MaxRoundsCarried"),
                                   script->GetDouble("ShotGun_FiringFreq"),
                                   script->GetDouble("ShotGun_IdealRange"),
                                   script->GetDouble("Pellet_MaxSpeed"),
                                   owner),

            m_iNumBallsInShell(script->GetInt("ShotGun_NumBallsInShell")),
            m_dSpread(script->GetDouble("ShotGun_Spread"))
{

    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, 0),
                                           Vector2D(0, -2),
                                           Vector2D(10, -2),
                                           Vector2D(10, 0),
                                           Vector2D(0, 0),
                                           Vector2D(0, 2),
                                           Vector2D(10, 2),
                                           Vector2D(10, 0)
                                           };

  
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}

//------------------------------ ShootAt --------------------------------------

inline void ShotGun::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //a shotgun cartridge contains lots of tiny metal balls called pellets. 
    //Therefore, every time the shotgun is discharged we have to calculate
    //the spread of the pellets and add one for each trajectory
    for (int b=0; b<m_iNumBallsInShell; ++b)
    {
      //determine deviation from target using a bell curve type distribution
      double deviation = RandInRange(0, m_dSpread) + RandInRange(0, m_dSpread) - m_dSpread;

      Vector2D AdjustedTarget = pos - m_pOwner->Pos();
 
      //rotate the target vector by the deviation
      Vec2DRotateAroundOrigin(AdjustedTarget, deviation);
 
      //add a pellet to the game world
      m_pOwner->GetWorld()->AddShotGunPellet(m_pOwner, AdjustedTarget + m_pOwner->Pos());

    }

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("ShotGun_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
inline double ShotGun::GetDesirability(double DistToTarget, double PerpendicularSpeed)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
	m_FuzzyModule.Fuzzify("TangencialSpeed", min(PerpendicularSpeed, 100));
    m_FuzzyModule.Fuzzify("DistanceToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//--------------------------- InitializeFuzzyModule ---------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void ShotGun::InitializeFuzzyModule()
{  
  FuzzyVariable& DistanceToTarget = m_FuzzyModule.CreateFLV("DistanceToTarget");

  FuzzyVariable& TanSpeed = m_FuzzyModule.CreateFLV("TangencialSpeed");
  FzSet& TanS_Fast = TanSpeed.AddRightShoulderSet("Fast", 3.5, 5, 1000);
  FzSet& TanS_Medium = TanSpeed.AddTriangularSet("Medium", 1.5, 3, 4.5);
  FzSet& TanS_Slow = TanSpeed.AddLeftShoulderSet("Slow", 0, 1, 2.5);

  FzSet& Target_Close = DistanceToTarget.AddLeftShoulderSet("Target_Close", 0, 25, 150);
  FzSet& Target_Medium = DistanceToTarget.AddTriangularSet("Target_Medium", 25, 150, 300);
  FzSet& Target_Far = DistanceToTarget.AddRightShoulderSet("Target_Far", 150, 300, 1000);

  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
  
  FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 50, 75, 100);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 25, 50, 75);
  FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 25, 50);

  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
  FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 30, 60, 100);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 0, 30, 60);
  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 0, 30);


  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Fast), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Fast), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Fast), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Fast), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Fast), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Fast), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Fast), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Fast), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, TanS_Fast), FzVery(Undesirable));

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Medium), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Medium), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Medium), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Medium), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Medium), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Medium), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Medium), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Medium), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, TanS_Medium), FzVery(Undesirable));

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads, TanS_Slow), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay, TanS_Slow), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low, TanS_Slow), FzVery(VeryDesirable));

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads, TanS_Slow), FzVery(VeryDesirable));
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay, TanS_Slow), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low, TanS_Slow), Desirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads, TanS_Slow), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay, TanS_Slow), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low, TanS_Slow), FzVery(Undesirable));
}

//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void ShotGun::Render()
{
  m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->BrownPen();

  gdi->PolyLine(m_vecWeaponVBTrans);

}