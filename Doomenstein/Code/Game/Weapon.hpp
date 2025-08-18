#pragma once
//#include "Game/ActorHandle.hpp"
#include "Game/GameCommon.hpp"
//#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include <string>

struct WeaponAnimationInfo;


class Weapon
{
public:
	Weapon(std::string weaponName);
	~Weapon();
	bool Fire(Actor* owner);

	void OnEquip();
	void OnUnequip();

	void FireRayWeapon(Actor* owner);
	void FireProjectileWeapon(Actor* owner);
	void FireMeleeWeapon(Actor* owner);
	void FireSpecialMeleeWeapon(Actor* owner);
	void FireSpecialRadarWeapon(Actor* owner);

	float GetAttackRange() const;

	void PlayDefaultAnimation();
	bool IsCurrentAnimationFinished() const;
	AABB2 GetCurrentSpriteUV() const;

	bool IsSpecialWeapon() const;
	float GetElapsedFractionOfSwitchWeapon() const;
public:
	WeaponDefinition const* m_definition = nullptr;
	//Map* m_map = nullptr;
	//ActorHandle m_actorHandle = ActorHandle::INVALID;
	WeaponAnimationInfo* m_currentAnimation = nullptr;


private:
	Vec3 GetRandomDirectionInCone(Actor* actor, float variationDegrees);
	void PlayAnimation(std::string const& name);

private:
	Clock* m_animationClock = nullptr;
	Timer m_fireTimer;
	Timer m_switchWeaponTimer;
	SoundPlaybackID m_firePlaybackID = MISSING_SOUND_ID;
	// TODO enemy need weapon animation?
};

