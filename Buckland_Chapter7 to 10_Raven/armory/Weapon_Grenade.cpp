#include "Weapon_Grenade.h"
#include "../Raven_Bot.h"
#include "../../Common/misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "../../Common/fuzzy/FuzzyOperators.h"

//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------

Weapon_Grenade::Weapon_Grenade(Raven_Bot * owner):Raven_Weapon(type_grenade,
												  script->GetInt("Grenade_DefaultRounds"),
												  script->GetInt("Grenade_MaxRoundsCarried"),
												  script->GetDouble("Blaster_FiringFreq"),
												  script->GetDouble("Grenade_IdealRange"),
												  script->GetDouble("Bolt_MaxSpeed"),
												  owner)
{
	//setup the vertex buffer
	const int NumWeaponVerts = 8;
	const Vector2D weapon[NumWeaponVerts] = { Vector2D(0, -3),
											  Vector2D(6, -3),
											  Vector2D(6, -1),
											  Vector2D(15, -1),
											  Vector2D(15, 1),
											  Vector2D(6, 1),
											  Vector2D(6, 3),
										      Vector2D(0, 3)
	};
	for (int vtx = 0; vtx<NumWeaponVerts; ++vtx)
	{
		m_vecWeaponVB.push_back(weapon[vtx]);
	}

	//setup the fuzzy module
	InitializeFuzzyModule();

}

//------------------------------ ShootAt --------------------------------------

inline void Weapon_Grenade::ShootAt(Vector2D pos)
{
	if (isReadyForNextShot())
	{
		//Fire in the hole!
		m_pOwner->GetWorld()->AddGrenade(m_pOwner, pos);

		m_iNumRoundsLeft--;

		UpdateTimeWeaponIsNextAvailable();

		//add a trigger to the game so that the other bots can hear this shot
		//(provided they are within range)
		m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("Grenade_SoundRange"));
	}
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------

double Weapon_Grenade::GetDesirability(double DistToTarget, double PerpendicularSpeed)
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

void Weapon_Grenade::InitializeFuzzyModule()
{
	FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

	FzSet& Target_Close = DistToTarget.AddLeftShoulderSet("Target_Close", 0, 25, 150);
	FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium", 25, 150, 300);
	FzSet& Target_Far = DistToTarget.AddRightShoulderSet("Target_Far", 150, 300, 1000);

	FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
	FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 50, 75, 100);
	FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 25, 50, 75);
	FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 25, 50);

	FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
	FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 10, 30, 100);
	FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 0, 10, 30);
	FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 0, 10);


	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), VeryDesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
}

//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void Weapon_Grenade::Render()
{
	m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
										m_pOwner->Pos(),
										m_pOwner->Facing(),
										m_pOwner->Facing().Perp(),
										m_pOwner->Scale());

	gdi->PurplePen();

	gdi->ClosedShape(m_vecWeaponVBTrans);
}