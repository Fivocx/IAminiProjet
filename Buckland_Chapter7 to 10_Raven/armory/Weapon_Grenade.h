#ifndef GRENADE_H
#define GRENADE_H
#pragma warning (disable:4786)

#include "Raven_Weapon.h"

class  Raven_Bot;

class Weapon_Grenade : public Raven_Weapon
{
private:
	void InitializeFuzzyModule();

public:
	Weapon_Grenade(Raven_Bot* owner);

	void  Render();

	void  ShootAt(Vector2D pos);

	double GetDesirability(double DistToTarget);
};

#endif