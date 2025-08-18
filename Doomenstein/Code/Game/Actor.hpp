#pragma once
#include "Game/GameCommon.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/Weapon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <vector>

class Actor
{
public:
	Actor() = default;
	~Actor();
	Actor(Map* map, SpawnInfo const& spawnInfo, ActorHandle handle);

	void Update(float deltaSeconds);
	//void UpdatePhysics(float fixedDeltaSeconds); // after all physics update and endPhysics update, update collision in world?
	//void EndUpdatePhysics(float fixedDeltaSeconds); // apply acceleration and velocity and clear acceleration

	void UpdatePhysics(float deltaSeconds); // todo make fixed update physics after math assignment
	// consider addForce/moveindirection in update/fixedupdate


	void Damage(float damageAmount, Actor* damageCauser);
	void Die();

	void AddForce(Vec3 const& force);
	void AddImpulse(Vec3 const& impulse);

	void OnCollide(Actor* other);
	void OnPossessed(Controller* controller);
	void OnUnpossessed();

	// Controller Move Input
	void MoveInDirection(Vec3 const& direction, float speed);
	void TurnInDirection(float targetOrientationDegrees, float maxDeltaDegrees);

	void Attack(); // fire current
	void EquipWeapon(int weaponIndex);
	void EquipPrevWeapon();
	void EquipNextWeapon();
	Weapon* GetEquippedWeapon() const;

	void Render() const;
	Mat44 GetModelToWorldTransform() const;

	bool IsOpposingFaction(Faction targetFaction) const;
	Vec2 GetForwardNormal2D() const;
	Vec3 GetEyePosition() const;
	Vec3 GetFirePosition(Vec3 offset = Vec3::ZERO) const;
	void GetThirdPersonCameraPositionAndOrientation(Vec3& camPosition, EulerAngles& camOrientation);
	float GetAttackRange() const;
	Vec3 GetVelocity() const;

	void SetInvisible();
	//void CreateBuffers();

	void PlayAnimation(std::string const& name);
	float GetAnimationDuration(std::string const& name) const;
	void PlaySFX(std::string const& name);

public:
	ActorHandle				m_handle = ActorHandle::INVALID;
	ActorHandle				m_owner = ActorHandle::INVALID; // only for projectile actors
	ActorDefinition const*	m_definition = nullptr;
	Map*					m_map = nullptr;


	Vec3		m_position;
	EulerAngles m_orientation;

	bool m_isDead		= false;
	bool m_isGarbage	= false;
	float m_health		= 77.f;

	Controller* m_controller = nullptr; // currently possessing this actor
	Controller* m_aiController = nullptr; // if aiEnabled new AI() for it, original ai controller

	Rgba8 m_color;

	//bool m_isStatic = false; // Not movable change to simulated
	// Stagger
	bool m_isStaggering = false;

private:
	Vec3	m_velocity;
	Vec3	m_acceleration;
	Timer	m_corpseTimer;

	std::vector<Weapon*> m_weapons;
	int m_currentWeaponIndex = -1; // if no weapon?

	//std::vector<Vertex_PCU> m_unlitVertexes;
	//std::vector<Vertex_PCUTBN> m_litVertexes;
	//VertexBuffer* m_vertexBuffer = nullptr; // Remember to delete it
	//IndexBuffer* m_indexBuffer = nullptr; // Remember to delete it

	// Animation
	ActorDefinition::AnimationGroup* m_currentAnimationGroup = nullptr;
	Clock* m_animationClock = nullptr;

	// Sound
	SoundPlaybackID m_hurtPlaybackID = MISSING_SOUND_ID;
	SoundPlaybackID m_deathPlaybackID = MISSING_SOUND_ID;

	bool m_isVisible = true;
};

