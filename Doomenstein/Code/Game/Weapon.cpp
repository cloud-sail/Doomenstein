#include "Game/Weapon.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"
#include "Game/Sound.hpp"
#include "Game/Player.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Game/MapDefinition.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Weapon::Weapon(std::string weaponName)
{
	m_definition = WeaponDefinition::GetByName(weaponName);

	m_animationClock = new Clock(*(g_theGame->m_clock));
	m_currentAnimation = m_definition->GetDefaultAnimationInfo();
	//m_map = owner->m_map;
	//m_actorHandle = owner->m_handle;
}


Weapon::~Weapon()
{
	delete m_animationClock;
}

bool Weapon::Fire(Actor* owner)
{
	if (m_fireTimer.IsStopped() || !m_fireTimer.HasPeriodElapsed())
	{
		return false;
	}

	if (!m_switchWeaponTimer.IsStopped() && !m_switchWeaponTimer.HasPeriodElapsed())
	{
		return false;
	}
	else
	{
		m_switchWeaponTimer.Stop();
	}

	//g_theAudio->StartSoundAt(m_definition->m_fireSFX, owner->GetFirePosition());
	g_theAudio->StartSound(m_definition->m_fireSFX, false, 0.4f);

	FireRayWeapon(owner);
	FireProjectileWeapon(owner);
	FireMeleeWeapon(owner);
	FireSpecialMeleeWeapon(owner);
	FireSpecialRadarWeapon(owner);

	m_fireTimer.Start(); // reset timer
	PlayAnimation("Attack");

	if (IsSpecialWeapon())
	{
		g_theGame->ActivateSpecialMusicVolume();
	}


	return true;
}

void Weapon::OnEquip()
{
	m_fireTimer = Timer(m_definition->m_refireTime, g_theGame->m_clock);
	//m_fireTimer.Start(); // TODO Add InitialStartDelay (-4) enable start shoot quicker.
	m_fireTimer.m_startTime = 0.01f;
	m_switchWeaponTimer = Timer(m_definition->m_switchWeaponSeconds, g_theGame->m_clock);
	m_switchWeaponTimer.Start();

	if (IsSpecialWeapon())
	{
		g_theGame->SetPlayBackSpeed(0.f);
		g_theGame->ResumeSpecialMusic();
	}

}

void Weapon::OnUnequip()
{
	if (IsSpecialWeapon())
	{
		g_theGame->SetPlayBackSpeed(1.f);
		g_theGame->PauseSpecialMusic();
		g_theGame->DeactivateSpecialMusicVolume();
	}
}

void Weapon::FireRayWeapon(Actor* owner)
{
	if (owner == nullptr)
	{
		return;
	}

	for (int i = 0; i < m_definition->m_rayCount; ++i)
	{
		Vec3 direction = GetRandomDirectionInCone(owner, m_definition->m_rayCone);
		Vec3 startPos = owner->GetFirePosition();
		RaycastResultWithActor result = owner->m_map->RaycastAll(startPos, direction, m_definition->m_rayRange, owner);
		if (result.m_hitActor != nullptr && !result.m_hitActor->m_isDead)
		{
			// Damage and Impulse and Notify AI
			float damage = g_rng.RollRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
			result.m_hitActor->Damage(damage, owner);
			result.m_hitActor->AddImpulse(direction * m_definition->m_rayImpulse);
		}

		//DebugDrawRaycastResult3D(result);

		if (result.m_hitActor) // Hit Actor
		{
			SpawnInfo info;
			info.m_actor = "BloodSplatter";
			info.m_position = result.m_impactPos;
			owner->m_map->SpawnActor(info);
		}
		else if (result.m_didImpact) // Hit something other than actor 
		{
			SpawnInfo info;
			info.m_actor = "BulletHit";
			info.m_position = result.m_impactPos;
			owner->m_map->SpawnActor(info);
		}
	}

}

void Weapon::FireProjectileWeapon(Actor* owner)
{
	if (owner == nullptr)
	{
		return;
	}

	for (int i = 0; i < m_definition->m_projectileCount; ++i)
	{
		Vec3 direction = GetRandomDirectionInCone(owner, m_definition->m_projectileCone);
		Vec3 startPos  = owner->GetFirePosition();

		SpawnInfo info;
		info.m_actor = m_definition->m_projectileActor;
		info.m_position = startPos;
		info.m_orientation = owner->m_orientation;
		info.m_velocity = direction * m_definition->m_projectileSpeed;
		Actor* projectileActor = owner->m_map->SpawnActor(info);
		projectileActor->m_owner = owner->m_handle;
	}

}


void Weapon::FireMeleeWeapon(Actor* owner)
{
	if (owner == nullptr)
	{
		return;
	}

	for (int i = 0; i < m_definition->m_meleeCount; ++i)
	{
		int numActor = (int)owner->m_map->m_allActors.size();
		for (int actorIndex = 0; actorIndex < numActor; ++actorIndex)
		{
			Actor* actor = owner->m_map->m_allActors[actorIndex];

			if (actor == nullptr || actor->m_isDead)
			{
				continue;
			}
			// only attack opposing faction
			if (!owner->IsOpposingFaction(actor->m_definition->m_faction))
			{
				continue;
			}

			if (IsPointInsideDirectedSector2D(Vec2(actor->m_position.x, actor->m_position.y), Vec2(owner->m_position.x, owner->m_position.y), owner->GetForwardNormal2D(), m_definition->m_meleeArc, m_definition->m_meleeRange))
			{
				float damage = g_rng.RollRandomFloatInRange(m_definition->m_meleeDamage.m_min, m_definition->m_meleeDamage.m_max);
				actor->Damage(damage, owner);
				Vec3 direction = (actor->m_position - owner->m_position).GetNormalized();
				actor->AddImpulse(direction * m_definition->m_meleeImpulse);
			}
		}
	}
}

void Weapon::FireSpecialMeleeWeapon(Actor* owner)
{
	if (owner == nullptr)
	{
		return;
	}

	if (m_definition->m_specialMeleeCount > 0)
	{
		Player* player = nullptr;
		if (owner->m_controller && owner->m_controller->IsPlayerController())
		{
			player = dynamic_cast<Player*>(owner->m_controller);
		}

		float approximateMoveDistance = 0.f;
		Vec2 ownerVelocity2D = Vec2(owner->GetVelocity().x, owner->GetVelocity().y);
		Vec2 ownerDashDirection = Vec2::ZERO;
		if (ownerVelocity2D.GetLengthSquared() > 1.f)
		{
			ownerDashDirection = ownerVelocity2D.GetNormalized();
			float averageSpeed = (ownerVelocity2D.GetLength() + m_definition->m_dashImpulse + owner->m_definition->m_physics.m_runSpeed) * 0.5f;
			float approximateTime = 1.9f / owner->m_definition->m_physics.m_drag;
			approximateMoveDistance = averageSpeed * approximateTime;
			g_theAudio->StartSound(Sound::PLAYER_DASH, false, 0.2f);
		}
		owner->AddImpulse(m_definition->m_dashImpulse * Vec3(ownerDashDirection));
		if (player)
		{
			player->AddSpeedLinesTrauma(1.0f);
		}


		Vec2 attackStartPos2D = Vec2(owner->m_position.x, owner->m_position.y);
		Vec2 attackEndPos2D = attackStartPos2D + approximateMoveDistance * ownerDashDirection;
		Capsule2 attackRange = Capsule2(attackStartPos2D, attackEndPos2D, m_definition->m_meleeRange);
		if (g_isDebugDraw)
		{
			DebugAddWorldWireSphere(Vec3(attackStartPos2D, 0.5f), m_definition->m_meleeRange, 3.f, Rgba8::RED, Rgba8::OPAQUE_WHITE);
			DebugAddWorldWireSphere(Vec3(attackEndPos2D, 0.5f), m_definition->m_meleeRange, 3.f, Rgba8::RED, Rgba8::OPAQUE_WHITE);
		}

		int numActor = (int)owner->m_map->m_allActors.size();
		for (int actorIndex = 0; actorIndex < numActor; ++actorIndex)
		{
			Actor* actor = owner->m_map->m_allActors[actorIndex];

			if (actor == nullptr || actor->m_isDead)
			{
				continue;
			}
			// only attack opposing faction
			if (!owner->IsOpposingFaction(actor->m_definition->m_faction))
			{
				continue;
			}

			if (IsPointInsideCapsule2D(Vec2(actor->m_position.x, actor->m_position.y), attackRange))
			{
				//float damage = g_rng.RollRandomFloatInRange(m_definition->m_meleeDamage.m_min, m_definition->m_meleeDamage.m_max);
				float damage = 10000.f;
				actor->Damage(damage, owner);
				actor->SetInvisible();


				if (player)
				{
					player->SetThirdPersonTimer(owner->GetAnimationDuration("GloryKill"));
				}
				
				owner->PlayAnimation("GloryKill");
			}
		}


	}

	//for (int i = 0; i < m_definition->m_specialMeleeCount; ++i)
	//{
	//}
}

void Weapon::FireSpecialRadarWeapon(Actor* owner)
{
	if (owner == nullptr)
	{
		return;
	}

	if (m_definition->m_specialRadarCount > 0)
	{
		Player* player = nullptr;
		if (owner->m_controller && owner->m_controller->IsPlayerController())
		{
			player = dynamic_cast<Player*>(owner->m_controller);
		}
		if (player)
		{
			player->AddRadarScanTrauma(1.f);
		}
	}
}

float Weapon::GetAttackRange() const
{
	float attackRange = 0.f;
	if (m_definition->m_projectileCount > 0)
	{
		attackRange = 15.f; // TODO in weapon definition
	}
	if (m_definition->m_rayCount > 0 && m_definition->m_rayRange > attackRange)
	{
		attackRange = m_definition->m_rayRange;
	}
	if (m_definition->m_meleeCount > 0 && m_definition->m_meleeRange > attackRange)
	{
		attackRange = m_definition->m_meleeRange;
	}
	return attackRange;
}

void Weapon::PlayDefaultAnimation()
{
	WeaponAnimationInfo* animInfo = m_definition->GetDefaultAnimationInfo();
	if (animInfo && animInfo != m_currentAnimation)
	{
		m_currentAnimation = animInfo;
		m_animationClock->Reset();
	}

	if (IsSpecialWeapon())
	{
		g_theGame->DeactivateSpecialMusicVolume();
	}
}

bool Weapon::IsCurrentAnimationFinished() const
{
	// Assume default is never finished and other animation are once
	if (m_currentAnimation == nullptr)
	{
		return true;
	}

	if (m_currentAnimation == m_definition->GetDefaultAnimationInfo())
	{
		return false;
	}

	return m_currentAnimation->m_spriteAnimDef->GetDuration() < m_animationClock->GetTotalSeconds();
}

AABB2 Weapon::GetCurrentSpriteUV() const
{
	if (m_currentAnimation == nullptr)
	{
		return AABB2();
	}

	SpriteDefinition const& spriteDef = m_currentAnimation->m_spriteAnimDef->GetSpriteDefAtTime((float)m_animationClock->GetTotalSeconds());
	return spriteDef.GetUVs();
}

bool Weapon::IsSpecialWeapon() const
{
	return m_definition->m_specialMeleeCount > 0;
}

float Weapon::GetElapsedFractionOfSwitchWeapon() const
{
	if (m_switchWeaponTimer.IsStopped())
	{
		return 1.f;
	}
	return GetClampedZeroToOne((float)m_switchWeaponTimer.GetElapsedFraction());
}

Vec3 Weapon::GetRandomDirectionInCone(Actor* actor, float variationDegrees)
{
	Mat44 rotMat = actor->m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	Vec3 iBasis = rotMat.GetIBasis3D();
	Vec3 jBasis = rotMat.GetJBasis3D();
	Vec3 kBasis = rotMat.GetKBasis3D();

	if (variationDegrees >= 180.f)
	{
		variationDegrees = 180.f;
	}

	float halfConeDegrees = 0.5f * variationDegrees;
	float i = CosDegrees(halfConeDegrees);
	float maxDeviationLength = SinDegrees(halfConeDegrees);

	float randomDegrees = g_rng.RollRandomFloatInRange(0.f, 360.f);
	float randomRadius = g_rng.RollRandomFloatInRange(0.f, maxDeviationLength);

	Vec2 jk = Vec2::MakeFromPolarDegrees(randomDegrees, randomRadius);

	return (i * iBasis + jk.x * jBasis + jk.y * kBasis).GetNormalized();
}

void Weapon::PlayAnimation(std::string const& name)
{
	WeaponAnimationInfo* animInfo = m_definition->GetAnimationInfoByName(name);
	if (animInfo)
	{
		// can restart the same animation again
		m_currentAnimation = animInfo;
		m_animationClock->Reset();
	}
}
