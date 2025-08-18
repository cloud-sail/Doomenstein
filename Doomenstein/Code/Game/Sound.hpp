#pragma once

#include "Engine/Audio/AudioSystem.hpp"

class Sound
{
public:
	static SoundID AI_ALERT;
	static SoundID PLAYER_DASH;
	static SoundID HIT_ENEMY;
	static SoundID ENTER_STAGGER;
	static SoundID SLAYER_ACTIVATED;


public:
	static void Init();
};
