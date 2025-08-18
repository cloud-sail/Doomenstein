#pragma once
#include "Game/ActorHandle.hpp"
#include "Game/GameCommon.hpp"

class Controller
{
public:
	enum class Type
	{
		PLAYER,
		AI,
		UNKNOWN,
	};

	Controller() = default;
	virtual ~Controller() = default;

	virtual void Update(float deltaSeconds) = 0;

	virtual void Possess(Actor* newActor);
	virtual Actor* GetActor() const;
	virtual bool IsPlayerController() const;
	virtual bool IsAIController() const;
	virtual Type GetType() const;

public:
	ActorHandle m_actorHandle = ActorHandle::INVALID;
	Map* m_map = nullptr;

};
