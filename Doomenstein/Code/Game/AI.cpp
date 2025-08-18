#include "Game/AI.hpp"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/Map.hpp"
#include "Game/Sound.hpp"
#include "Engine/Math/MathUtils.hpp"

AI::AI(ActorDefinition::AIInfo aiInfo)
	: m_staggerDuration(aiInfo.m_staggerSeconds)
	, m_damageThreshold(aiInfo.m_staggerDamageThreshold)
{

}

Controller::Type AI::GetType() const
{
	return Type::AI;
}

void AI::Update(float deltaSeconds)
{
	Actor* controlledActor = GetActor();
	if (controlledActor == nullptr)
	{
		return;
	}

	if (controlledActor->m_isDead)
	{
		return;
	}

	switch (m_currentState)
	{
	case AIState::IDLE:
		HandleIdleState(deltaSeconds);
		break;
	case AIState::FLEE:
		HandleFleeState(deltaSeconds);
		break;
	case AIState::STAGGER:
		HandleStaggerState(deltaSeconds);
		break;
	default:
		break;
	}
}

void AI::DamagedBy(float damangeAmount, Actor* damageCauser)
{
	if (damageCauser == nullptr)
	{
		return;
	}

	//Actor* selfActor = m_map->GetActorByHandle(m_actorHandle);
	Actor* selfActor = GetActor();
	if (selfActor == nullptr) // prevent error
	{
		return;
	}

	//if (!selfActor->m_isStaggering) // Or not in staggering state
	if (m_currentState != AIState::STAGGER && !selfActor->m_isDead)
	{
		m_accumulatedDamange += damangeAmount;
		if (m_accumulatedDamange >= m_damageThreshold)
		{
			// Enter Stagger State
			m_currentState = AIState::STAGGER;
			m_accumulatedDamange = 0.f;
			selfActor->m_isStaggering = true;
			m_staggerTimer = m_staggerDuration;
			g_theAudio->StartSoundAt(Sound::ENTER_STAGGER, selfActor->GetEyePosition());
		}
	}

	// Check owner
	if (damageCauser->m_owner.IsValid())
	{
		Actor* ownerActor = m_map->GetActorByHandle(damageCauser->m_owner);
		if (ownerActor == nullptr) // prevent error
		{
			return;
		}

		if (selfActor->IsOpposingFaction(ownerActor->m_definition->m_faction))
		{
			m_targetActorHandle = damageCauser->m_owner;
		}

		Player* player = dynamic_cast<Player*>(ownerActor->m_controller);
		if (player)
		{
			player->AddHitTrauma(1.f);
		}

		return;
	}
	
	// check actor
	if (selfActor->IsOpposingFaction(damageCauser->m_definition->m_faction))
	{
		m_targetActorHandle = damageCauser->m_handle;
	}
	Player* player = dynamic_cast<Player*>(damageCauser->m_controller);
	if (player)
	{
		player->AddHitTrauma(1.f);
	}
}

void AI::HandleIdleState(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Actor* controlledActor = GetActor();
	if (controlledActor == nullptr)
	{
		return;
	}
	Actor* currentTargetActor = m_map->GetActorByHandle(m_targetActorHandle);
	if (currentTargetActor == nullptr)
	{
		// no target: Search
		Actor* newTargetActor = m_map->GetClosestVisibleEnemy(controlledActor);
		if (newTargetActor != nullptr)
		{
			m_targetActorHandle = newTargetActor->m_handle;
			m_currentState = AIState::FLEE;
			g_theAudio->StartSoundAt(Sound::AI_ALERT, controlledActor->GetEyePosition());
		}
	}
	else
	{
		m_currentState = AIState::FLEE;
		g_theAudio->StartSoundAt(Sound::AI_ALERT, controlledActor->GetEyePosition());
	}
}

void AI::HandleFleeState(float deltaSeconds)
{
	// If person who chase you is dead, back to idle?

	Actor* controlledActor = GetActor();
	if (controlledActor == nullptr)
	{
		return;
	}
	// No target, back to idle
	Actor* currentTargetActor = m_map->GetActorByHandle(m_targetActorHandle);
	if (currentTargetActor == nullptr || currentTargetActor->m_isDead)
	{
		m_targetActorHandle = ActorHandle::INVALID;
		m_currentState = AIState::IDLE;
		return;
	}

	Vec2 moveDirection = m_map->GetBilinearInterpResultFromWorldPos(Vec2(controlledActor->m_position.x, controlledActor->m_position.y));
	controlledActor->TurnInDirection(moveDirection.GetOrientationDegrees(), deltaSeconds * controlledActor->m_definition->m_physics.m_turnSpeed);
	controlledActor->MoveInDirection(Vec3(moveDirection), controlledActor->m_definition->m_physics.m_runSpeed);
	//Vec2 selfForward2D = controlledActor->GetForwardNormal2D();
	//controlledActor->MoveInDirection(Vec3(selfForward2D), controlledActor->m_definition->m_physics.m_runSpeed);

}

void AI::HandleStaggerState(float deltaSeconds)
{
	m_staggerTimer -= deltaSeconds;
	Actor* selfActor = m_map->GetActorByHandle(m_actorHandle);
	if (selfActor)
	{
		if (m_staggerTimer < 0.f)
		{
			selfActor->m_isStaggering = false;
			m_currentState = AIState::FLEE;
		}
	}
}

//void AI::TransitionToState(AIState newState)
//{
//	if (m_currentState == newState)
//	{
//		return;
//	}
//
//	m_currentState = newState;
//
//	switch (m_currentState)
//	{
//	case AIState::IDLE:
//		break;
//	case AIState::FLEE:
//		break;
//	case AIState::STAGGER:
//		break;
//	default:
//		break;
//	}
//}
