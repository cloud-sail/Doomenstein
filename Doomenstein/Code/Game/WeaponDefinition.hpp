#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include <vector>
#include <string>

struct WeaponAnimationInfo
{
	std::string m_name;
	Shader* m_shader = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;

	SpriteAnimDefinition* m_spriteAnimDef = nullptr;
};

struct WeaponDefinition
{
	static void InitializeDefinitions(const char* path = "Data/Definitions/WeaponDefinitions.xml");
	static void ClearDefinitions();
	static WeaponDefinition const* GetByName(std::string const& weaponDefName);
	static std::vector<WeaponDefinition*> s_definitions;

	bool LoadFromXmlElement(XmlElement const& element);

	std::string m_name = "UnknownWeapon";
	float m_refireTime = 0.f;

	// ray weapon
	int m_rayCount = 0;
	float m_rayCone = 0.f; // angle variation
	float m_rayRange = 0.f;
	FloatRange m_rayDamage = FloatRange::ZERO;
	float m_rayImpulse = 0.0f;

	// projectile weapon
	int m_projectileCount = 0;
	float m_projectileCone = 0.f;
	float m_projectileSpeed = 0.f;
	std::string m_projectileActor = "";

	// melee weapon
	int m_meleeCount = 0;
	float m_meleeArc = 0.f;
	float m_meleeRange = 0.f;
	FloatRange m_meleeDamage = FloatRange::ZERO;
	float m_meleeImpulse = 0.f;

	int m_specialMeleeCount = 0;
	float m_dashImpulse = 0.f;

	int m_specialRadarCount = 0;

	// Sounds
	SoundID m_fireSFX = MISSING_SOUND_ID;

	// HUD
	Shader* m_hudShader = nullptr; // for HUD and reticle
	Texture* m_baseTexture = nullptr;
	Texture* m_reticleTexture = nullptr;
	Vec2 m_reticleSize = Vec2(1.f, 1.f);
	Vec2 m_spriteSize = Vec2(1.f, 1.f); // weapon size
	Vec2 m_spritePivot = Vec2(0.5f, 0.f);

	// Animation
	std::vector<WeaponAnimationInfo*> m_animations;
	float m_switchWeaponSeconds = 1.0f;

	WeaponAnimationInfo* GetAnimationInfoByName(std::string const& name) const;
	WeaponAnimationInfo* GetDefaultAnimationInfo() const;
};

