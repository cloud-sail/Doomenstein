#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Controller.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/Camera.hpp"

class BitmapFont;

class Player : public Controller
{
public:
	Player();
	~Player() = default;

	void Update(float deltaSeconds) override;
	void Possess(Actor* newActor) override;
	Type GetType() const override;

	void RenderScreen() const;

	void OnControlledActorDie();

	void SetThirdPersonTimer(float seconds);
	bool IsUsingThirdPersonCamera() const;

	void AddHitTrauma(float stress);
	void AddSpeedLinesTrauma(float stress);
	void AddRadarScanTrauma(float stress);


protected:
	void UpdateInput();
	void UpdateFreeFlyInput();
	void UpdateFirstPersonInput();
	void UpdateFirstPersonInputByKeyboard(Actor* controlledActor);
	void UpdateFirstPersonInputByController(Actor* controlledActor);
	void UpdateSinglePlayerCheats();
	void UpdateThirdPersonInput();
	void UpdateRadarScanResult();

	void AttemptLockOn();
	void UnlockTarget();
	Actor* GetNearestEnemyToCenter() const;

	void UpdateCamera();
	void UpdateAudioListener();
	void UpdateAnimation();
	void UpdateWeaponShake();




	void DebugDrawInfo();

public:
	Camera m_screenCamera; // For HUD
	Camera m_camera;
	Vec3 m_position;
	EulerAngles m_orientation;


	bool m_isFreeFlyMode = false;

	Timer m_dyingTimer;


	int m_playerIndex = -1;
	int m_controllerIndex = KEYBOARD_AND_MOUSE; // -1 for keyBoard 0+ for Controller
	int m_numKills = 0;
	int m_numDeaths = 0;
	AABB2 m_normalizedViewport = AABB2::ZERO_TO_ONE;

protected:
	// Weapon Shake
	Vec2 m_weaponShakeOffset;
	
	float m_thirdPersonTimer = 0.f; // if Timer>0.f means using third person camera

	float m_radarScanTrauma = 0.f;

	float m_hitTrauma = 0.f;
	float m_speedLinesTrauma = 0.f;
	Texture* m_hitTexture = nullptr;
	Texture* m_scannedBoxTexture = nullptr;
	Texture* m_speedLinesTexture = nullptr;
	Texture* m_lockOnTexture = nullptr;
	BitmapFont* m_font = nullptr;


	ActorHandle m_targetActorHandle = ActorHandle::INVALID;

	ActorHandle m_closestToCenterEnemy = ActorHandle::INVALID; // It will be update every frame
	bool m_isLockOn = false;

	std::vector<Vec2> m_scannedEnemyViewportPoints;
	std::vector<std::string> m_scannedEnemyDescriptions;
};

