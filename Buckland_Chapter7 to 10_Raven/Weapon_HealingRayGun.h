#pragma once

#include "armory\Raven_Weapon.h"

class HealingRayGun : public Raven_Weapon
{

private:
	void  InitializeFuzzyModule();

public:
	HealingRayGun(Raven_Bot* owner);
	void  Render();

	void  ShootAt(Vector2D pos);

	double GetDesirability(double DistToTarget, double TangentialSpeed);
};



