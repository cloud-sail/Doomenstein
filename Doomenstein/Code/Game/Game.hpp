#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

//-----------------------------------------------------------------------------------------------
enum class GameState
{
	DEFAULT,
	ATTRACT,
	LOBBY,
	PLAYING,
	VICTORY,
	COUNT
};


// ----------------------------------------------------------------------------------------------
class Game
{
public:
	Game();
	~Game();
	void Update();
	void Render() const;

	Map* GetCurrentMap() const;
	bool IsMultiplayerMode() const;
	float GetPlayerCameraAspect() const;

public:
	Clock* m_clock = nullptr;

	std::vector<Player*> m_players;
	mutable Player* m_currentRenderingPlayer = nullptr; // only be used and changed in Render stage

private:
	void UpdateDeveloperCheats();
	void AdjustForPauseAndTimeDistortion();

	void RenderUI() const;
	void UpdateCameras();

	void CreatePlayers();
	void DestroyPlayers();
	void CreateMaps();
	void DestroyMaps();

	void DebugDrawUpdate();

	void LoadAudio();

private:
	//std::vector<Map*> m_maps; // Only one maps now
	Map* m_currentMap = nullptr; 

	Camera m_screenCamera;

	Skybox* m_skybox;

//-----------------------------------------------------------------------------------------------
// Background Music Control
public:
	void ResumeSpecialMusic();
	void PauseSpecialMusic();
	void ActivateSpecialMusicVolume();
	void DeactivateSpecialMusicVolume();

	void PlayMusic(SoundID soundID, float volume = 0.25f);
	void SetPlayBackSpeed(float playbackSpeed);
	void StopMusic();



private:
	SoundPlaybackID m_musicPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_specialMusicPlaybackID = MISSING_SOUND_ID;

//-----------------------------------------------------------------------------------------------
private:
	void ResetLighting();
	void ApplyLighting();
	void UpdateLighting();

private:
	Vec3 m_sunDirection;
	float m_sunIntensity;
	float m_ambientIntensity;

//-----------------------------------------------------------------------------------------------
// Game State Transition
public:
	void SetNextState(GameState newState);
	bool IsCurrentState(GameState state) const;
	
private:
	void SwitchToNextState();
	void EnterState(GameState state);
	void ExitState(GameState state);

	void UpdateCurrentState();
	void RenderCurrentState() const;

	void UpdateStateAttract();
	void UpdateStateLobby();
	void UpdateStatePlaying();
	void UpdateStateVictory();

	void RenderStateAttract() const;
	void RenderStateLobby() const;
	void RenderStatePlaying() const;
	void RenderStateVictory() const;

private:
	GameState m_currentState = GameState::DEFAULT;
	GameState m_nextState = GameState::DEFAULT; // Change the state and trigger Enter and Exit at the beginning

	bool m_isPlayer1UsingKeyBoard = true;
	bool m_isPlayer1Taken = false;
	bool m_isPlayer2Taken = false;

	//int m_specialMusicResumeRefCount = 0;
	//int m_specialMusicActivateRefCount = 0;

};

