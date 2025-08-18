#pragma once
#include "Game/Controller.hpp"
#include "Game/ActorDefinition.hpp"

enum class AIState
{
	IDLE,
	FLEE,
	STAGGER,
};


class AI : public Controller
{
public:
	AI(ActorDefinition::AIInfo aiInfo);
	Type GetType() const override;
	
	void Update(float deltaSeconds) override;
	void DamagedBy(float damangeAmount, Actor* damageCauser);


public:
	ActorHandle m_targetActorHandle = ActorHandle::INVALID;

protected:
	AIState m_currentState = AIState::IDLE;

private:
	// Coward AI
	void HandleIdleState(float deltaSeconds);
	void HandleFleeState(float deltaSeconds);
	void HandleStaggerState(float deltaSeconds);

	//void TransitionToState(AIState newState);

private:
	float const m_staggerDuration = 3.0f; // will be changed by actor definition
	float m_staggerTimer = 0.f;
	float const m_damageThreshold = 30.f; // will be changed by actor definition
	float m_accumulatedDamange = 0.f;
};

