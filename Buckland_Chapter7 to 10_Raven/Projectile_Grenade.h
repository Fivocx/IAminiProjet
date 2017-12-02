#ifndef ROCKET_H
#define ROCKET_H
#pragma warning (disable:4786)
//-----------------------------------------------------------------------------
//
//  Name:   Rocket.h
//
//  Author: Mat Buckland (ai-junkie.com)
//
//  Desc:   class to implement a rocket
//
//-----------------------------------------------------------------------------

#include "Raven_Projectile.h"

class Raven_Bot;
class Projectile_Grenade :
	public Raven_Projectile
{
public:
	Projectile_Grenade();
	~Projectile_Grenade();
};

