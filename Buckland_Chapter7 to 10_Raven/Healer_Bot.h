#pragma once
#include "Raven_Bot.h"
class Healer_Bot : public Raven_Bot
{
public:
	Healer_Bot(Raven_Game* world, Vector2D pos, bool isleader, bool autreEquipe, Raven_Bot* leaderPt);
	~Healer_Bot();
	
	void        Update();
	void		Render();

	double HealerGoalsPriority[8] = {
		1,	//HealthBias,
		0,	//ShotgunBias,
		0,	//RocketLauncherBias,
		0,	//RailgunBias,
		0,	//GrenadeBias,
		0,	//ExploreBias,
		1,	//AttackBias,
		1 };	//AttackLeaderBias,
};

