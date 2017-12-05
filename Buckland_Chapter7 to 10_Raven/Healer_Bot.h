#pragma once
#include "Raven_Bot.h"
class Healer_Bot : public Raven_Bot
{
public:
	Healer_Bot(Raven_Game* world, Vector2D pos, bool isleader, bool autreEquipe, Raven_Bot* leaderPt);
	~Healer_Bot();
	
	void        Update();
	void		Render();
};

