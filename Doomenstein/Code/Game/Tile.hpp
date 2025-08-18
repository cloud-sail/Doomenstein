#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Math/AABB3.hpp"


struct Tile
{
	AABB3 m_bounds;
	TileDefinition const* m_tileDef = nullptr;
};

