#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include <vector>
#include <string>

enum class Faction
{
	NEUTRAL,
	MARINE,
	DEMON,
};


Faction StringToFaction(std::string const& factionString);
BillboardType StringToBillboardType(std::string const& billBoardType);
SpriteAnimPlaybackType StringToSpriteAnimPlaybackType(std::string const& playbackType);

// struct Animation
// struct AnimationGroup
// struct VisualsInfo
// struct SoundInfo

//-----------------------------------------------------------------------------------------------
struct ActorDefinition
{
	static void InitializeDefinitions(std::vector<std::string> const& paths = {"Data/Definitions/ActorDefinitions.xml", "Data/Definitions/ProjectileActorDefinitions.xml"});
	static void ClearDefinitions();
	static ActorDefinition const* GetByName(std::string const& actorDefName);
	static std::vector<ActorDefinition*> s_definitions;

	bool LoadFromXmlElement(XmlElement const& element);


	struct CollisionInfo
	{
		float	m_physicsRadius = 0.f;
		float	m_physicsHeight = 0.f;
		bool	m_collidesWithWorld = false;
		bool	m_collidesWithActors = false;
		bool	m_dieOnCollide = false;
		FloatRange m_damageOnCollide = FloatRange::ZERO;
		float	m_impulseOnCollide = 0.f;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct PhysicsInfo
	{
		bool	m_simulated = false;
		bool	m_flying = false;
		float	m_walkSpeed = 0.f;
		float	m_runSpeed = 0.f;
		float	m_drag = 0.f;
		float	m_turnSpeed = 0.f;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct CameraInfo
	{
		float m_eyeHeight = 0.f;
		float m_cameraFOVDegrees = 60.f;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct AIInfo
	{
		bool	m_aiEnabled = false;
		float	m_sightRadius = 0.f;
		float	m_sightAngle = 0.f;
		float	m_staggerSeconds = 0.f;
		float	m_staggerDamageThreshold = 1000.f;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct InventoryInfo
	{
		std::vector<std::string> m_weapons;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct AnimationGroup // todo GetAnimGroupByName(std::string const& name)
	{
		std::string		m_name;
		bool			m_scaleBySpeed = false;
		std::vector<Vec3> m_directions;
		std::vector<SpriteAnimDefinition> m_spriteAnimDefs;
		SpriteAnimPlaybackType m_playbackType;

		bool LoadFromXmlElement(XmlElement const& element, SpriteSheet const& spriteSheet);

		const SpriteAnimDefinition& GetAnimationForDirection(Vec3 const& localViewDirection) const;
		bool IsAnimationFinished(float seconds) const;

	};

	struct VisualsInfo
	{
		Vec2			m_size = Vec2(1.f, 1.f);
		Vec2			m_pivot = Vec2(0.5f, 0.5f);
		BillboardType	m_billboardType = BillboardType::NONE;
		bool			m_renderLit = false;
		bool			m_renderRounded = false;
		Shader*			m_shader = nullptr;
		SpriteSheet*	m_spriteSheet = nullptr;
		IntVec2			m_cellCount = IntVec2(1, 1);

		std::vector<AnimationGroup*> m_animationGroups;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	struct SoundsInfo
	{
		SoundID m_hurtSFX = MISSING_SOUND_ID;
		SoundID m_deathSFX = MISSING_SOUND_ID;

		bool LoadFromXmlElement(XmlElement const& element);
	};

	AnimationGroup* GetAnimationGroupByName(std::string const& name) const;
	AnimationGroup* GetDefaultAnimationGroup() const;

	std::string m_name = "UNKNOWNACTOR";
	bool		m_isVisible = false;
	float		m_health = 1.f;
	float		m_corpseLifetime = 0.f;
	Faction		m_faction = Faction::NEUTRAL;
	bool		m_canBePossessed = false;
	bool		m_dieOnSpawn = false;

	CollisionInfo	m_collision;
	PhysicsInfo		m_physics;
	CameraInfo		m_camera;
	AIInfo			m_ai;
	InventoryInfo	m_inventory;
	VisualsInfo		m_visuals;
	SoundsInfo		m_sounds;

	

};

