#pragma once
#include "Triggers\Trigger.h"
#include "Raven_Bot.h"
#include "Raven_WeaponSystem.h"
#include "lua\Raven_Scriptor.h"
#include "Raven_ObjectEnumerations.h"
#include "Weapon_HealingRayGun.h"
#include "armory\Weapon_Blaster.h"
#include <iosfwd>
#include <cassert>
#include <vector>

class Trigger_SpawnLoot :public Trigger<Raven_Bot>
{
public:
	std::vector<Raven_Weapon*> weapons = std::vector<Raven_Weapon*>();
	Trigger_SpawnLoot(unsigned GraphNodeIndex, Vector2D pos):Trigger<Raven_Bot>(GetNextValidID())
	{
		SetPos(pos);
		SetBRadius(7);
		SetGraphNodeIndex(GraphNodeIndex);

		//create this trigger's region of fluence
		AddCircularTriggerRegion(Pos(), script->GetDouble("DefaultGiverTriggerRange"));

		SetEntityType(type_health);
	}

	virtual void Update()
	{
			// Isn't just great!?
	}


	void AddWeapon(Raven_Weapon* weapon)
	{
		if (dynamic_cast<Blaster*>(weapon) != nullptr || dynamic_cast<HealingRayGun*>(weapon) != nullptr)
			return;
		weapons.push_back(weapon);
		SetActive();
	}

	void Try(Raven_Bot* pBot)
	{
		if (weapons.size() == 0)
			return;

		if (this->isActive() && this->isTouchingTrigger(pBot->Pos(), pBot->BRadius()))
		{
			std::vector<Raven_Weapon*>::iterator curWeapon = weapons.begin();
			for(curWeapon; curWeapon != weapons.end(); curWeapon++)
				pBot->GetWeaponSys()->AddWeapon(*curWeapon);
			weapons.clear();
		}
		SetInactive();
	}


	void Render()
	{
		int wCount = weapons.size();
		if (wCount == 0) return;
		gdi->GreenBrush();
		gdi->BrownPen();
		const double sz = 3.0;

		gdi->Circle(Pos().x - sz, Pos().y, sz);
		if (wCount == 1)return;
		gdi->Circle(Pos().x + sz, Pos().y, sz);
		if (wCount == 2)return;
		gdi->Circle(Pos().x , Pos().y + sz, sz);
		if (wCount == 3)return;
		gdi->Circle(Pos().x , Pos().y - sz, sz);

	}


};

