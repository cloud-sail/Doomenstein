#include "Game/Sound.hpp"
#include "Game/GameCommon.hpp"

SoundID Sound::AI_ALERT = MISSING_SOUND_ID;
SoundID Sound::PLAYER_DASH = MISSING_SOUND_ID;
SoundID Sound::HIT_ENEMY = MISSING_SOUND_ID;
SoundID Sound::ENTER_STAGGER = MISSING_SOUND_ID;
SoundID Sound::SLAYER_ACTIVATED = MISSING_SOUND_ID;

void Sound::Init()
{
	AI_ALERT = g_theAudio->CreateOrGetSound("Data/Audio/AIAlert.wav", true);
	PLAYER_DASH = g_theAudio->CreateOrGetSound("Data/Audio/Dash.wav");
	HIT_ENEMY = g_theAudio->CreateOrGetSound("Data/Audio/HitEnemy.wav");
	ENTER_STAGGER = g_theAudio->CreateOrGetSound("Data/Audio/EnterStagger.wav", true);
	SLAYER_ACTIVATED = g_theAudio->CreateOrGetSound("Data/Audio/SlayerActivated.wav");
}
