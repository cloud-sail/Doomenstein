#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/AI.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Gradient.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include <vector>

Actor::Actor(Map* map, SpawnInfo const& spawnInfo, ActorHandle handle)
	: m_map(map)
	, m_handle(handle)
	, m_position(spawnInfo.m_position)
	, m_orientation(spawnInfo.m_orientation)
	, m_velocity(spawnInfo.m_velocity)
{
	m_definition = ActorDefinition::GetByName(spawnInfo.m_actor);
	m_animationClock = new Clock(*(g_theGame->m_clock));
	m_currentAnimationGroup = m_definition->GetDefaultAnimationGroup();

	if (m_definition->m_dieOnSpawn)
	{
		Die();
	}

	// load weapons
	for (std::string weaponName : m_definition->m_inventory.m_weapons)
	{
		m_weapons.push_back(new Weapon(weaponName));
	}
	if (!m_weapons.empty())
	{
		EquipWeapon(0);
	}

	// Reset health
	m_health = m_definition->m_health;

	// AI Controller
	if (m_definition->m_ai.m_aiEnabled)
	{
		m_aiController = new AI(m_definition->m_ai);

		m_aiController->Possess(this);
	}

	// Visualize Collision
	//if (m_definition->m_isVisible)
	//{
	//	AddVertsForCylinderZ3D(m_unlitVertexes, Vec2::ZERO, FloatRange(0.f, m_definition->m_collision.m_physicsHeight), m_definition->m_collision.m_physicsRadius, 16);
	//	if (!m_definition->m_physics.m_flying)
	//	{
	//		float beakHeight = m_definition->m_camera.m_eyeHeight * 0.8f;
	//		float xOffset = m_definition->m_collision.m_physicsRadius;
	//		AddVertsForCone3D(m_unlitVertexes, Vec3(xOffset, 0.f, beakHeight), Vec3(xOffset * 1.4f, 0.f, beakHeight), xOffset * 0.4f, Rgba8::OPAQUE_WHITE, AABB2::ZERO_TO_ONE, 16);
	//	}
	//}

	// Temp Code
	//if (spawnInfo.m_actor == "Marine")
	//{
	//	m_color = Rgba8::GREEN;
	//}
	//else if (spawnInfo.m_actor == "Demon")
	//{
	//	m_color = Rgba8::RED;
	//}
	//else
	//{
	//	m_color = Rgba8::BLUE;
	//}
}

Actor::~Actor()
{
	delete m_aiController;
	delete m_animationClock;

	for (Weapon* weapon : m_weapons)
	{
		weapon->OnUnequip();
		delete weapon;
	}
	m_weapons.clear();

	//delete m_vertexBuffer;
	//m_vertexBuffer = nullptr;
	//delete m_indexBuffer;
	//m_indexBuffer = nullptr;
}

void Actor::Update(float deltaSeconds)
{
	if (m_isGarbage)
	{
		return; // error?
	}
	//-----------------------------------------------------------------------------------------------
	// Update AI Controller
	if (m_controller != nullptr && m_controller->IsAIController())
	{
		m_controller->Update(deltaSeconds);
	}

	//-----------------------------------------------------------------------------------------------
	// Update Animation
	if (m_currentAnimationGroup->m_scaleBySpeed)
	{
		m_animationClock->SetTimeScale(m_velocity.GetLength() / m_definition->m_physics.m_runSpeed);
	}
	else
	{
		m_animationClock->SetTimeScale(1.0);
	}

	if (m_currentAnimationGroup == nullptr || (m_currentAnimationGroup != m_definition->GetDefaultAnimationGroup() && m_currentAnimationGroup->IsAnimationFinished((float)m_animationClock->GetTotalSeconds()) &&  m_currentAnimationGroup->m_name != "Death"))
	{
		// Play Default Animation
		m_currentAnimationGroup = m_definition->GetDefaultAnimationGroup();
		m_animationClock->Reset();
	}

	//-----------------------------------------------------------------------------------------------
	// Update Physics
	UpdatePhysics(deltaSeconds);

	//-----------------------------------------------------------------------------------------------
	// Update Audio
	if (g_theAudio->IsChannelPlaying(m_hurtPlaybackID))
	{
		g_theAudio->SetSoundPosition(m_hurtPlaybackID, m_position);
	}

	if (g_theAudio->IsChannelPlaying(m_deathPlaybackID))
	{
		g_theAudio->SetSoundPosition(m_deathPlaybackID, m_position);
	}

	//-----------------------------------------------------------------------------------------------
	// Update Destroyed Status
	if (m_corpseTimer.HasPeriodElapsed())
	{
		m_isGarbage = true;
		if (m_controller && m_controller->IsPlayerController())
		{
			m_map->OnPlayerDie();
		}
	}

}

void Actor::UpdatePhysics(float deltaSeconds)
{
	if (!m_definition->m_physics.m_simulated || m_isDead)
	{
		return;
	}
	AddForce(-m_velocity * m_definition->m_physics.m_drag);

	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;
	m_acceleration = Vec3::ZERO;

	if (!m_definition->m_physics.m_flying)
	{
		m_position.z = 0.f; // force on ground if not flying
	}

	// TODO gravity?
}

void Actor::Damage(float damageAmount, Actor* damageCauser)
{
	if (m_isDead || m_isGarbage)
	{
		return;
	}
	// Immune damage from same faction?
	// TODO update death and kills? 

	m_health -= damageAmount;
	
	if (m_health <= 0.f)
	{
		m_health = 0.f;

		// Update Kills and Deaths
		if (m_definition->m_faction != Faction::NEUTRAL)
		{

			Player* victim = dynamic_cast<Player*>(m_controller);
			if (victim)
			{
				victim->m_numDeaths++;
			}

			if (damageCauser)
			{
				if (damageCauser->m_owner.IsValid())
				{
					Actor* attacker = m_map->GetActorByHandle(damageCauser->m_owner);
					if (attacker->m_controller && attacker->m_controller->IsPlayerController())
					{
						Player* killer = dynamic_cast<Player*>(attacker->m_controller);
						if (killer)
						{
							killer->m_numKills++;
						}
					}
				}
				else
				{
					Actor* attacker = damageCauser;
					if (attacker->m_controller && attacker->m_controller->IsPlayerController())
					{
						Player* killer = dynamic_cast<Player*>(attacker->m_controller);
						if (killer)
						{
							killer->m_numKills++;
						}
					}
				}
			}

		}


		//// Update Kills and Deaths
		//if (g_theGame->IsMultiplayerMode())
		//{
		//	if (m_controller && m_controller->IsPlayerController())
		//	{
		//		if (damageCauser)
		//		{
		//			if (damageCauser->m_owner.IsValid())
		//			{
		//				Actor* attacker = m_map->GetActorByHandle(damageCauser->m_owner);
		//				if (attacker->m_controller && attacker->m_controller->IsPlayerController())
		//				{
		//					Player* killer = dynamic_cast<Player*>(attacker->m_controller);
		//					Player* victim = dynamic_cast<Player*>(m_controller);
		//					if (killer && victim)
		//					{
		//						killer->m_numKills++;
		//						victim->m_numDeaths++;
		//					}
		//				}
		//			}
		//			else
		//			{
		//				Actor* attacker = damageCauser;
		//				if (attacker->m_controller && attacker->m_controller->IsPlayerController())
		//				{
		//					Player* killer = dynamic_cast<Player*>(attacker->m_controller);
		//					Player* victim = dynamic_cast<Player*>(m_controller);
		//					if (killer && victim)
		//					{
		//						killer->m_numKills++;
		//						victim->m_numDeaths++;
		//					}
		//				}
		//			}
		//		}
		//	}
		//}

		Die();
	}
	else
	{
		PlayAnimation("Hurt");
		PlaySFX("Hurt");
	}

	if (m_controller && m_controller->IsAIController())
	{
		AI* aiController = dynamic_cast<AI*>(m_controller);
		aiController->DamagedBy(damageAmount, damageCauser);
	}

}

void Actor::Die()
{
	if (m_isDead)
	{
		return;
	}

	m_isDead = true;
	m_corpseTimer = Timer(m_definition->m_corpseLifetime, g_theGame->m_clock);
	m_corpseTimer.Start();
	PlayAnimation("Death");
	PlaySFX("Death");

	if (m_controller && m_controller->IsPlayerController())
	{
		Player* player = dynamic_cast<Player*>(m_controller);
		if (player)
		{
			player->OnControlledActorDie();
		}
	}

	if (m_definition->m_faction == Faction::DEMON)
	{
		m_map->ShowNumOfAliveDemons();
	}
}

void Actor::AddForce(Vec3 const& force)
{
	m_acceleration += force; // TODO differenet mass?
}

void Actor::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

void Actor::OnCollide(Actor* other)
{
	// can die, do damage, apply impulse...
	if (other != nullptr && !other->m_isDead)
	{
		FloatRange damageRange = m_definition->m_collision.m_damageOnCollide;
		if (damageRange != FloatRange::ZERO)
		{
			float damage = g_rng.RollRandomFloatInRange(damageRange.m_min, damageRange.m_max);
			other->Damage(damage, this);

			//if (other->m_controller && other->m_controller->IsAIController())
			//{
			//	AI* aiController = dynamic_cast<AI*>(other->m_controller);
			//	aiController->DamagedBy(this);
			//}
		}

		Vec3 impulseDirection = Vec3(m_velocity.x, m_velocity.y, 0.f).GetNormalized();
		other->AddImpulse(impulseDirection * m_definition->m_collision.m_impulseOnCollide);
	}

	if (m_definition->m_collision.m_dieOnCollide)
	{
		Die();
	}
}

void Actor::OnPossessed(Controller* controller)
{
	m_controller = controller;

	// TODO change to a new AI controller?
}

void Actor::OnUnpossessed()
{
	m_controller = m_aiController;
}

void Actor::MoveInDirection(Vec3 const& direction, float speed)
{
	AddForce(direction * speed * m_definition->m_physics.m_drag);
}

void Actor::TurnInDirection(float targetOrientationDegrees, float maxDeltaDegrees)
{
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, targetOrientationDegrees, maxDeltaDegrees);
}

void Actor::Attack()
{
	if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= (int)m_weapons.size())
	{
		return;
	}
	m_weapons[m_currentWeaponIndex]->Fire(this); // TODO fail to fire no bullet or not cooled down
}

void Actor::EquipWeapon(int weaponIndex)
{
	if (weaponIndex < 0 || weaponIndex >= (int)m_weapons.size())
	{
		return;
	}
	if (m_currentWeaponIndex != weaponIndex)
	{
		if (m_currentWeaponIndex > 0 && m_currentWeaponIndex <= (int)m_weapons.size())
		{
			m_weapons[m_currentWeaponIndex]->OnUnequip();
		}
		m_currentWeaponIndex = weaponIndex;
		m_weapons[weaponIndex]->OnEquip();
	}
}

void Actor::EquipPrevWeapon()
{
	if (m_weapons.empty())
	{
		return;
	}
	int numWeapon = (int)m_weapons.size();
	EquipWeapon((m_currentWeaponIndex - 1 + numWeapon) % numWeapon);
}

void Actor::EquipNextWeapon()
{
	if (m_weapons.empty())
	{
		return;
	}
	int numWeapon = (int)m_weapons.size();
	EquipWeapon((m_currentWeaponIndex + 1) % numWeapon);
}

Weapon* Actor::GetEquippedWeapon() const
{
	if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= (int)m_weapons.size())
	{
		return nullptr;
	}
	return m_weapons[m_currentWeaponIndex];
}

void Actor::Render() const
{
	if (!m_definition->m_isVisible)
	{
		return;
	}
	// Manually set invisible
	if (!m_isVisible)
	{
		return;
	}

	// Avoid Render Self, but later we need to show the actor when doing a glory kill, but it is freefly mode? no freefly mode is a debug method
	if (m_controller && m_controller->IsPlayerController())
	{
		Player* player = dynamic_cast<Player*>(m_controller);
		if (player && player == g_theGame->m_currentRenderingPlayer && !player->m_isFreeFlyMode)
		{
			if (!player->IsUsingThirdPersonCamera())
			{
				return;
			}
		}
	}

	ActorDefinition::VisualsInfo const& visual = m_definition->m_visuals;
	Camera const& currentCamera = g_theGame->m_currentRenderingPlayer->m_camera;

	// Get Model Matrix
	Mat44 modelToWorldTransform;
	if (visual.m_billboardType == BillboardType::NONE)
	{
		modelToWorldTransform = GetModelToWorldTransform();
	}
	else
	{
		modelToWorldTransform = GetBillboardTransform(visual.m_billboardType, currentCamera.GetCameraToWorldTransform(), m_position);
	}
	g_theRenderer->SetModelConstants(modelToWorldTransform);
	
	// Get facing sprite UVs
	Vec3 cameraToActor = m_position - currentCamera.GetPosition();
	Vec3 localViewDirection = GetModelToWorldTransform().GetOrthonormalInverse().TransformVectorQuantity3D(cameraToActor);
	SpriteAnimDefinition const& animDef = m_currentAnimationGroup->GetAnimationForDirection(localViewDirection);
	SpriteDefinition const& spriteDef = animDef.GetSpriteDefAtTime((float)m_animationClock->GetTotalSeconds());
	AABB2 UVs = spriteDef.GetUVs();

	// Create Geometry
	AABB2 bounds = AABB2(Vec2::ZERO, visual.m_size);
	bounds.Translate(visual.m_size * -visual.m_pivot);
	if (visual.m_renderLit && !m_isStaggering)
	{
		std::vector<Vertex_PCUTBN> verts;
		verts.reserve(12);
		if (visual.m_renderRounded)
		{
			AddVertsForRoundedQuad3D(verts, Vec3(0.f, bounds.m_mins.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_maxs.y), Vec3(0.f, bounds.m_mins.x, bounds.m_maxs.y), Rgba8::OPAQUE_WHITE, UVs);
		}
		else
		{
			AddVertsForQuad3D(verts, Vec3(0.f, bounds.m_mins.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_maxs.y), Vec3(0.f, bounds.m_mins.x, bounds.m_maxs.y), Rgba8::OPAQUE_WHITE, UVs);
		}
		
		g_theRenderer->BindTexture(&visual.m_spriteSheet->GetTexture());
		g_theRenderer->BindShader(visual.m_shader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		g_theRenderer->DrawVertexArray(verts);
	}
	else
	{
		std::vector<Vertex_PCU> verts;
		verts.reserve(12);

		// For stagger using unlit default shader
		Rgba8 color = Rgba8::OPAQUE_WHITE;
		if (m_isStaggering)
		{
			float currentSeconds = (float)g_theGame->m_clock->GetTotalSeconds();

			Gradient colorGradient;
			colorGradient.SetKeys({
				GradientRgba8Key(0.0f,	Rgba8::OPAQUE_WHITE),
				GradientRgba8Key(1.0f,	Rgba8(0, 0, 255)),
				});
			color = colorGradient.Evaluate(LinearSine(currentSeconds, 0.5f));
		}


		AddVertsForQuad3D(verts, Vec3(0.f, bounds.m_mins.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_mins.y), Vec3(0.f, bounds.m_maxs.x, bounds.m_maxs.y), Vec3(0.f, bounds.m_mins.x, bounds.m_maxs.y), color, UVs);
		
		g_theRenderer->BindTexture(&visual.m_spriteSheet->GetTexture());
		//g_theRenderer->BindShader(visual.m_shader);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

		g_theRenderer->DrawVertexArray(verts);
	}


	//Rgba8 tintColor = m_color;
	//if (m_isDead)
	//{
	//	tintColor = Interpolate(Rgba8(0, 0, 0), tintColor, 0.4f);
	//}

	//g_theRenderer->SetModelConstants(GetModelToWorldTransform(), tintColor);
	//g_theRenderer->BindTexture(nullptr);
	//g_theRenderer->BindShader(nullptr);
	//g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	//g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	//g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	//g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	//g_theRenderer->DrawVertexArray(m_unlitVertexes);

	//// Wire Frame
	//Rgba8 lightColor = Interpolate(tintColor, Rgba8::OPAQUE_WHITE, 0.8f);
	//g_theRenderer->SetModelConstants(GetModelToWorldTransform(), lightColor);
	//g_theRenderer->BindTexture(nullptr);
	//g_theRenderer->BindShader(nullptr);
	//g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	//g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	//g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	//g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	//g_theRenderer->DrawVertexArray(m_unlitVertexes);
}

Mat44 Actor::GetModelToWorldTransform() const
{
	EulerAngles modelOrientation = EulerAngles(m_orientation.m_yawDegrees, 0.f, 0.f);
	Mat44 result = modelOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	result.SetTranslation3D(m_position);
	return result;
}

bool Actor::IsOpposingFaction(Faction targetFaction) const
{
	if (m_definition->m_faction == Faction::DEMON)
	{
		return (targetFaction != Faction::NEUTRAL && targetFaction != Faction::DEMON);
	}
	else if (m_definition->m_faction == Faction::MARINE)
	{
		return (targetFaction != Faction::NEUTRAL && targetFaction != Faction::MARINE);
	}

	return false;
}

Vec2 Actor::GetForwardNormal2D() const
{
	return Vec2::MakeFromPolarDegrees(m_orientation.m_yawDegrees); // In AI also use yaw
}

Vec3 Actor::GetEyePosition() const
{
	return m_position + Vec3::UP * m_definition->m_camera.m_eyeHeight; // TODO floating while running
}

Vec3 Actor::GetFirePosition(Vec3 offset /*= Vec3::ZERO*/) const
{
	Vec2 temp = GetForwardNormal2D() * m_definition->m_collision.m_physicsRadius * 0.8f;
	Vec3 weaponOffset = Vec3(temp) - Vec3::UP * m_definition->m_camera.m_eyeHeight * 0.08f;
	return GetEyePosition() + weaponOffset + offset;
}

void Actor::GetThirdPersonCameraPositionAndOrientation(Vec3& camPosition, EulerAngles& camOrientation)
{
	Vec3 thirdPersonCameraMultiplier = Vec3(-1.2f, -1.2f, 0.8f);
	Vec2 forwardNormal2D = GetForwardNormal2D();
	camPosition = m_position + Vec3(forwardNormal2D, 1.f) * thirdPersonCameraMultiplier;
	Vec3 lookAtPoint = GetEyePosition();
	Mat44 camRotMat = Mat44::MakeFromX(lookAtPoint - camPosition);
	camOrientation = camRotMat.GetEulerAngles();
}

float Actor::GetAttackRange() const
{
	if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= (int)m_weapons.size())
	{
		return 0.f;
	}
	return m_weapons[m_currentWeaponIndex]->GetAttackRange();
}

Vec3 Actor::GetVelocity() const
{
	return m_velocity;
}

void Actor::SetInvisible()
{
	m_isVisible = false;
}

//void Actor::CreateBuffers()
//{
//	if (!m_definition->m_isVisible)
//	{
//		return;
//	}
//
//}

void Actor::PlayAnimation(std::string const& name)
{
	ActorDefinition::AnimationGroup* animGroup = m_definition->GetAnimationGroupByName(name);
	if (animGroup)
	{
		// Do not restart the same animation again, but what if hurt twice?
		if (animGroup != m_currentAnimationGroup)
		{
			m_currentAnimationGroup = animGroup;
			m_animationClock->Reset();
		}
	}
}

float Actor::GetAnimationDuration(std::string const& name) const
{
	// Only be used for glory kill
	ActorDefinition::AnimationGroup* animGroup = m_definition->GetAnimationGroupByName(name);
	if (animGroup)
	{
		return animGroup->m_spriteAnimDefs[0].GetDuration();
	}
	return 0.f;
}

void Actor::PlaySFX(std::string const& name)
{
	if (name == "Hurt")
	{
		m_hurtPlaybackID = g_theAudio->StartSoundAt(m_definition->m_sounds.m_hurtSFX, m_position);
	}
	else if (name == "Death")
	{
		m_deathPlaybackID = g_theAudio->StartSoundAt(m_definition->m_sounds.m_deathSFX, m_position, false, 0.5f);
	}
}
