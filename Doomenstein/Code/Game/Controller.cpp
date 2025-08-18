#include "Game/Controller.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"

void Controller::Possess(Actor* newActor)
{
	Actor* currentPossessedActor = GetActor();
	if (currentPossessedActor != nullptr)
	{
		currentPossessedActor->OnUnpossessed();
		m_actorHandle = ActorHandle::INVALID;
		m_map = nullptr;
	}

	if (newActor != nullptr)
	{
		m_actorHandle = newActor->m_handle;
		m_map = newActor->m_map;
		newActor->OnPossessed(this);
	}
}

Actor* Controller::GetActor() const
{
	if (m_map)
	{
		return m_map->GetActorByHandle(m_actorHandle);
	}
	return nullptr;
}

bool Controller::IsPlayerController() const
{
	return GetType() == Type::PLAYER;
}

bool Controller::IsAIController() const
{
	return GetType() == Type::AI;
}

Controller::Type Controller::GetType() const
{
	return Type::UNKNOWN;
}
