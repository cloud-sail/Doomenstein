#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/TileDefinition.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"


static constexpr float UNREACHABLE_VALUE = 1.f;
static constexpr float SPECIAL_VALUE_POS = 999999.f;
static constexpr float SPECIAL_VALUE_NEG = -1.f;
static constexpr float EXPOSED_VALUE = 10000.f;
static constexpr float UNEXPOSED_VALUE = 0.f;
static float SIGHT_RANGE = 15.f;

//-----------------------------------------------------------------------------------------------
Map::Map(Game* game, const MapDefinition* definition)
	: m_game(game)
	, m_definition(definition)
	, m_texture(definition->m_spriteSheetTexture)
	, m_shader(definition->m_shader)
{
	//delete g_theGame->m_player;
	//g_theGame->m_player = new Player();


	CreateTiles();
	CreateGeometry();
	CreateBuffers();
	CreateSpawnPoints();
	CreateNavGrids();
	SpawnNonPlayerActors();

	for (int playerIndex = 0; playerIndex < (int)g_theGame->m_players.size(); ++playerIndex)
	{
		SpawnPlayer(playerIndex);
	}
	ShowNumOfAliveDemons();
}

Map::~Map()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
	delete m_indexBuffer;
	m_indexBuffer = nullptr;

	for (Actor* actor : m_allActors)
	{
		delete actor;
	}
	m_allActors.clear();

	delete m_reachableMap;
	delete m_exposureMap;
	delete m_flowField;

}

//-----------------------------------------------------------------------------------------------
void Map::Update()
{
	UpdateActors();
	CollideActors();
	CollideActorsWithMap();
	DeleteDestroyedActors();
	CheckAndSpawnPlayers();
	UpdateNavGrids();
}

void Map::UpdateActors()
{
	float deltaSeconds = static_cast<float>(m_game->m_clock->GetDeltaSeconds());
	std::vector<Actor*> actorsCopy = m_allActors; // prevent possible infinite loop

	for (Actor* actor : actorsCopy)
	{
		if (actor != nullptr)
		{
			actor->Update(deltaSeconds);
		}
	}
}

void Map::CollideActors()
{
	int numActor = (int)m_allActors.size();
	for (int actorIndexA = 0; actorIndexA < numActor; ++actorIndexA)
	{
		if (!IsAlive(m_allActors[actorIndexA]))
		{
			continue;
		}
		for (int actorIndexB = actorIndexA + 1; actorIndexB < numActor; ++actorIndexB)
		{
			if (!IsAlive(m_allActors[actorIndexB]))
			{
				continue;
			}
			CollideActor(m_allActors[actorIndexA], m_allActors[actorIndexB]);
		}
	}
}



void Map::CollideActor(Actor* actorA, Actor* actorB)
{
	if (!actorA->m_definition->m_collision.m_collidesWithActors || !actorB->m_definition->m_collision.m_collidesWithActors)
	{
		return;
	}
	// projectile vs. projectile and projectile vs. owner
	// same owner
	if (actorA->m_owner == actorB->m_owner && actorA->m_owner.IsValid())
	{
		return;
	}
	// A owns B / B owns A
	if (actorA->m_owner == actorB->m_handle || actorB->m_owner == actorA->m_handle)
	{
		return;
	}

	FloatRange	zRangeA = FloatRange(actorA->m_position.z, actorA->m_position.z + actorA->m_definition->m_collision.m_physicsHeight);
	FloatRange	zRangeB = FloatRange(actorB->m_position.z, actorB->m_position.z + actorB->m_definition->m_collision.m_physicsHeight);
	float		radiusA = actorA->m_definition->m_collision.m_physicsRadius;
	float		radiusB = actorB->m_definition->m_collision.m_physicsRadius;
	Vec2		centerA = Vec2(actorA->m_position.x, actorA->m_position.y);
	Vec2		centerB = Vec2(actorB->m_position.x, actorB->m_position.y);

	//if (DoZCylindersOverlap3D(centerA, radiusA, zRangeA, centerB, radiusB, zRangeB))
	//{
	//	bool isSimulatedA = actorA->m_definition->m_physics.m_simulated;
	//	bool isSimulatedB = actorB->m_definition->m_physics.m_simulated;

	//	bool isPushed = false;
	//	if (isSimulatedA && isSimulatedB)
	//	{
	//		isPushed = PushDiscsOutOfEachOther2D(centerA, radiusA, centerB, radiusB);
	//	}
	//	else if (isSimulatedA && !isSimulatedB)
	//	{
	//		isPushed = PushDiscOutOfFixedDisc2D(centerA, radiusA, centerB, radiusB);
	//	}
	//	else if (!isSimulatedA && isSimulatedB)
	//	{
	//		isPushed = PushDiscOutOfFixedDisc2D(centerB, radiusB, centerA, radiusA);
	//	}

	//	if (isPushed) // must be true
	//	{
	//		actorA->m_position.x = centerA.x;
	//		actorA->m_position.y = centerA.y;
	//		actorB->m_position.x = centerB.x;
	//		actorB->m_position.y = centerB.y;

	//		actorA->OnCollide(actorB);
	//		actorB->OnCollide(actorA);
	//	}
	//}


	// To test calculate less
	//-----------------------------------------------------------------------------------------------
	if (zRangeA.IsOverlappingWith(zRangeB))
	{
		bool isSimulatedA = actorA->m_definition->m_physics.m_simulated;
		bool isSimulatedB = actorB->m_definition->m_physics.m_simulated;

		bool isPushed = false;
		if (isSimulatedA && isSimulatedB)
		{
			isPushed = PushDiscsOutOfEachOther2D(centerA, radiusA, centerB, radiusB);
		}
		else if (isSimulatedA && !isSimulatedB)
		{
			isPushed = PushDiscOutOfFixedDisc2D(centerA, radiusA, centerB, radiusB);
		}
		else if (!isSimulatedA && isSimulatedB)
		{
			isPushed = PushDiscOutOfFixedDisc2D(centerB, radiusB, centerA, radiusA);
		}

		if (isPushed)
		{
			actorA->m_position.x = centerA.x;
			actorA->m_position.y = centerA.y;
			actorB->m_position.x = centerB.x;
			actorB->m_position.y = centerB.y;

			actorA->OnCollide(actorB);
			actorB->OnCollide(actorA);
		}
	}
}

void Map::CollideActorsWithMap()
{
	// If it is static no need to collide with map?
	int numActor = (int)m_allActors.size();
	for (int actorIndex = 0; actorIndex < numActor; ++actorIndex)
	{
		if (!IsAlive(m_allActors[actorIndex]))
		{
			continue;
		}
		CollideActorWithMap(m_allActors[actorIndex]);
	}
}

void Map::CollideActorWithMap(Actor* actor)
{
	if (!actor->m_definition->m_collision.m_collidesWithWorld)
	{
		return;
	}

	// Collide with Grid
	IntVec2 currentCoords = GetCoordsForWorldPos(actor->m_position.x, actor->m_position.y);
	IntVec2 directions[8] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0), IntVec2(1,1), IntVec2(-1,1), IntVec2(-1,-1), IntVec2(1,-1) };
	for (int i = 0; i < 8; ++i)
	{
		IntVec2 tileCoords = currentCoords + directions[i];

		if (!IsTileSolid(tileCoords.x, tileCoords.y))
		{
			continue;
		}
		Vec2 mins = Vec2(static_cast<float>(tileCoords.x), static_cast<float>(tileCoords.y));
		AABB2 tileBounds = AABB2(mins, mins+ Vec2::ONE);

		Vec2 actorCenter = Vec2(actor->m_position.x, actor->m_position.y);
		bool isPushed = PushDiscOutOfFixedAABB2D(actorCenter, actor->m_definition->m_collision.m_physicsRadius, tileBounds);
		if (isPushed)
		{
			actor->m_position.x = actorCenter.x;
			actor->m_position.y = actorCenter.y;

			actor->OnCollide(nullptr);
		}
	}


	// Collide with Ceiling and Floor
	float constexpr CEILINGZ = 1.f;
	float constexpr FLOORZ = 0.f;

	float actorTopZ = actor->m_position.z + actor->m_definition->m_collision.m_physicsHeight;
	if (actorTopZ > CEILINGZ)
	{
		actor->m_position.z -= (actorTopZ - CEILINGZ);
		actor->OnCollide(nullptr);
	}

	float actorBottomZ = actor->m_position.z;
	if (actorBottomZ < FLOORZ)
	{
		actor->m_position.z += (FLOORZ - actorBottomZ);
		actor->OnCollide(nullptr);
	}
}

void Map::CheckAndSpawnPlayers()
{
	for (int playerIndex = 0; playerIndex < (int)g_theGame->m_players.size(); ++playerIndex)
	{
		Player* player = g_theGame->m_players[playerIndex];

		if (player->GetActor() == nullptr || player->m_map != this)
		{
			SpawnPlayer(playerIndex);
		}
	}
	// No possessed actor, wrong map
	// A Known BUG, when player possess other actor, and marine dead, it will not spawn new marine for player
}

void Map::UpdateNavGrids()
{
	UpdateExposureMap();
	UpdateFlowField();
}

void Map::UpdateExposureMap()
{
	std::vector<Vec2> playerPositions;

	// Get Player Positions
	for (int playerIndex = 0; playerIndex < (int)g_theGame->m_players.size(); ++playerIndex)
	{
		Player* player = g_theGame->m_players[playerIndex];
		Actor* controlledActor = player->GetActor();

		if (controlledActor != nullptr)
		{
			Vec3 playerPosition = controlledActor->m_position;
			playerPositions.push_back(Vec2(playerPosition.x, playerPosition.y));
		}
	}

	// Cast rays from each player to each cell, set initial value for exposure map
	m_exposureMap->SetAllValues(UNEXPOSED_VALUE);
	for (Vec2 const& playerPos : playerPositions)
	{
		for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
			{
				if (IsTileUnreachable(tileX, tileY)) // Use the reachable map
				{
					m_exposureMap->SetValueAtCoords(IntVec2(tileX, tileY), SPECIAL_VALUE_POS);
					continue;
				}

				Vec2 rayEnd = GetTileCenter(tileX, tileY);
				Vec2 disp = rayEnd - playerPos;
				RaycastResult2D result = FastVoxelRaycast(playerPos, disp.GetNormalized(), disp.GetLength());
				// Limited view range
				if (!result.m_didImpact && result.m_impactDist <= SIGHT_RANGE)
				{
					// player can reach
					m_exposureMap->SetValueAtCoords(IntVec2(tileX, tileY), EXPOSED_VALUE);
				}
			}
		}
	}

	//SpreadDistanceMapHeat(*m_exposureMap, UNEXPOSED_VALUE, 1.f);
	SpreadDistanceMapHeatOnReachableMap(*m_exposureMap, UNEXPOSED_VALUE, 1.f);

	// Enemy Will Run to the farthest tile
	// Reverse spread
	int numTiles = m_exposureMap->GetNumTiles();
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		if (m_exposureMap->GetValueAtIndex(tileIndex) == UNEXPOSED_VALUE)
		{
			m_exposureMap->SetValueAtIndex(tileIndex, SPECIAL_VALUE_NEG);
		}
	}

	//SpreadDistanceMapHeat(*m_exposureMap, UNEXPOSED_VALUE + 1.f, -1.f);
	SpreadDistanceMapHeatOnReachableMap(*m_exposureMap, UNEXPOSED_VALUE + 1.f, -1.f);
}

void Map::UpdateFlowField()
{
	IntVec2 directions[8] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0), IntVec2(1,1), IntVec2(-1,1), IntVec2(-1,-1), IntVec2(1,-1) };
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			IntVec2 flowDirection = IntVec2(0, 0);
			float minDelta = 0; // delta = neighborValue - currentValue

			IntVec2 currentTileCoords = IntVec2(tileX, tileY);
			float currentTileValue = m_exposureMap->GetValueAtCoords(currentTileCoords);
			bool isNeighborImpassable[4] = {};
			for (int i = 0; i < 4; ++i)
			{
				IntVec2 neighborTileCoords = currentTileCoords + directions[i];
				if (IsTileUnreachable(neighborTileCoords.x, neighborTileCoords.y))
				{
					isNeighborImpassable[i] = true;
					continue;
				}
				float neighborTileValue = m_exposureMap->GetValueAtCoords(neighborTileCoords);
				float delta = neighborTileValue - currentTileValue;
				if (delta < minDelta)
				{
					flowDirection = directions[i];
					minDelta = delta;
				}
			}

			for (int i = 4; i < 8; ++i)
			{
				// The Corner is not accessible
				if (i == 4 && isNeighborImpassable[0] && isNeighborImpassable[2]) continue;
				if (i == 5 && isNeighborImpassable[0] && isNeighborImpassable[3]) continue;
				if (i == 6 && isNeighborImpassable[1] && isNeighborImpassable[3]) continue;
				if (i == 7 && isNeighborImpassable[1] && isNeighborImpassable[2]) continue;

				IntVec2 neighborTileCoords = currentTileCoords + directions[i];
				if (!m_flowField->IsInBounds(neighborTileCoords))
				{
					continue;
				}
				float neighborTileValue = m_exposureMap->GetValueAtCoords(neighborTileCoords);
				float delta = neighborTileValue - currentTileValue;
				if (delta < minDelta)
				{
					flowDirection = directions[i];
					minDelta = delta;
				}
			}
			m_flowField->SetValueAtCoords(currentTileCoords, Vec2(flowDirection).GetNormalized());
		}
	}
}

RaycastResult2D Map::FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const
{
	RaycastResult2D raycastResult;
	raycastResult.m_ray.m_startPos = rayStart;
	raycastResult.m_ray.m_fwdNormal = rayForwardNormal;
	raycastResult.m_ray.m_maxLength = rayLength;

	int tileX = RoundDownToInt(rayStart.x);
	int tileY = RoundDownToInt(rayStart.y);

	if (IsTileSolid(tileX, tileY))
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
		return raycastResult;
	}

	float fwdDistPerXCrossing = 1.0f / fabsf(rayForwardNormal.x); // 1.f / cos theta
	int	tileStepDirectionX = (rayForwardNormal.x < 0.f) ? -1 : 1;
	float xAtFirstXCrossing = (static_cast<float>(tileX) + static_cast<float>(tileStepDirectionX + 1) * 0.5f);
	float xDistToFirstXCrossing = xAtFirstXCrossing - rayStart.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing; // forward direction distance

	float fwdDistPerYCrossing = 1.0f / fabsf(rayForwardNormal.y); // 1 / sin theta
	int tileStepDirectionY = (rayForwardNormal.y < 0.f) ? -1 : 1;
	float yAtFirstYCrossing = (static_cast<float>(tileY) + static_cast<float>(tileStepDirectionY + 1) * 0.5f);
	float yDistToFirstYCrossing = yAtFirstYCrossing - rayStart.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing; // forward direction distance

	while (true)
	{
		if (fwdDistAtNextXCrossing <= fwdDistAtNextYCrossing)
		{
			if (fwdDistAtNextXCrossing > rayLength)
			{
				raycastResult.m_impactDist = rayLength;
				return raycastResult;
			}
			tileX += tileStepDirectionX;
			if (IsTileSolid(tileX, tileY))
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = fwdDistAtNextXCrossing;
				raycastResult.m_impactPos = rayStart + raycastResult.m_impactDist * rayForwardNormal;
				raycastResult.m_impactNormal = Vec2(-static_cast<float>(tileStepDirectionX), 0.f);
				return raycastResult;
			}
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;
		}
		else
		{
			if (fwdDistAtNextYCrossing > rayLength)
			{
				raycastResult.m_impactDist = rayLength;
				return raycastResult;
			}
			tileY += tileStepDirectionY;
			if (IsTileSolid(tileX, tileY))
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = fwdDistAtNextYCrossing;
				raycastResult.m_impactPos = rayStart + raycastResult.m_impactDist * rayForwardNormal;
				raycastResult.m_impactNormal = Vec2(0.f, -static_cast<float>(tileStepDirectionY));
				return raycastResult;
			}
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;
		}
	}
}

void Map::SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep /*= 1.f*/)
{
	// Normally the spread step is +1, and Increasing Heat
	// Already set values, just do heat spreading
	IntVec2 const dimensions = distanceMap.m_dimensions;
	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };

	bool isHeatIncreasing = heatSpreadStep > 0.f;

	float currentSearchValue = startSearchValue;

	bool isHeatSpreading = true;

	while (isHeatSpreading)
	{
		isHeatSpreading = false;

		for (int tileY = 0; tileY < dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < dimensions.x; ++tileX)
			{
				IntVec2 const currentTileCoords = IntVec2(tileX, tileY);
				float currentTileValue = distanceMap.GetValueAtCoords(currentTileCoords);
				if (currentTileValue == currentSearchValue)
				{
					isHeatSpreading = true; // or on neighbor value changed, one more round of search
					for (int i = 0; i < 4; ++i) // four directions
					{
						IntVec2 neighborTileCoords = currentTileCoords + directions[i];
						if (!distanceMap.IsInBounds(neighborTileCoords))
						{
							continue;
						}
						if (IsTileSolid(neighborTileCoords.x, neighborTileCoords.y))
						{
							continue;
						}
						float neighborTileValue = distanceMap.GetValueAtCoords(neighborTileCoords);
						if (isHeatIncreasing && neighborTileValue <= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						if (!isHeatIncreasing && neighborTileValue >= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						distanceMap.SetValueAtCoords(neighborTileCoords, currentSearchValue + heatSpreadStep);
					}
				}
			}
		}
		currentSearchValue += heatSpreadStep;
	}
}

void Map::SpreadDistanceMapHeatOnReachableMap(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep /*= 1.f*/)
{
	// Normally the spread step is +1, and Increasing Heat
	// Already set values, just do heat spreading
	IntVec2 const dimensions = distanceMap.m_dimensions;
	IntVec2 directions[4] = { IntVec2(0,1), IntVec2(0,-1), IntVec2(1,0), IntVec2(-1,0) };

	bool isHeatIncreasing = heatSpreadStep > 0.f;

	float currentSearchValue = startSearchValue;

	bool isHeatSpreading = true;

	while (isHeatSpreading)
	{
		isHeatSpreading = false;

		for (int tileY = 0; tileY < dimensions.y; ++tileY)
		{
			for (int tileX = 0; tileX < dimensions.x; ++tileX)
			{
				IntVec2 const currentTileCoords = IntVec2(tileX, tileY);
				float currentTileValue = distanceMap.GetValueAtCoords(currentTileCoords);
				if (currentTileValue == currentSearchValue)
				{
					isHeatSpreading = true; // or on neighbor value changed, one more round of search
					for (int i = 0; i < 4; ++i) // four directions
					{
						IntVec2 neighborTileCoords = currentTileCoords + directions[i];
						if (!distanceMap.IsInBounds(neighborTileCoords))
						{
							continue;
						}
						if (IsTileUnreachable(neighborTileCoords.x, neighborTileCoords.y))
						{
							continue;
						}
						float neighborTileValue = distanceMap.GetValueAtCoords(neighborTileCoords);
						if (isHeatIncreasing && neighborTileValue <= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						if (!isHeatIncreasing && neighborTileValue >= (currentSearchValue + heatSpreadStep))
						{
							continue;
						}
						distanceMap.SetValueAtCoords(neighborTileCoords, currentSearchValue + heatSpreadStep);
					}
				}
			}
		}
		currentSearchValue += heatSpreadStep;
	}
}

bool Map::IsTileUnreachable(int tileX, int tileY) const
{
	IntVec2 const tileCoords = IntVec2(tileX, tileY);
	if (!m_reachableMap->IsInBounds(tileCoords))
	{
		return true;
	}

	return m_reachableMap->GetValueAtCoords(tileCoords) == UNREACHABLE_VALUE;
}

Vec2 Map::GetBilinearInterpResultFromWorldPos(Vec2 const& worldPos) const
{
	Vec2 posAtBottomLeftTileCoords = (worldPos - Vec2(0.5f, 0.5f));
	IntVec2 bottomLeftTileCoords = IntVec2(RoundDownToInt(posAtBottomLeftTileCoords.x), RoundDownToInt(posAtBottomLeftTileCoords.y));
	IntVec2 topLeftTileCoords = bottomLeftTileCoords + IntVec2(0, 1);
	IntVec2 bottmRightTileCoords = bottomLeftTileCoords + IntVec2(1, 0);
	IntVec2 topRightTileCoords = bottomLeftTileCoords + IntVec2(1, 1);

	Vec2 value00 = GetSafeValueFromFlowField(bottomLeftTileCoords);
	Vec2 value01 = GetSafeValueFromFlowField(topLeftTileCoords);
	Vec2 value10 = GetSafeValueFromFlowField(bottmRightTileCoords);
	Vec2 value11 = GetSafeValueFromFlowField(topRightTileCoords);

	float xWeight = posAtBottomLeftTileCoords.x - floorf(posAtBottomLeftTileCoords.x); // 0~1
	float yWeight = posAtBottomLeftTileCoords.y - floorf(posAtBottomLeftTileCoords.y);

	Vec2 topDirection = Interpolate(value01, value11, xWeight);
	Vec2 bottomDirection = Interpolate(value00, value10, xWeight);

	Vec2 resultDirection = Interpolate(bottomDirection, topDirection, yWeight);

	return resultDirection.GetNormalized();
}

Vec2 Map::GetSafeValueFromFlowField(IntVec2 tileCoords) const
{
	// It can not become engine code, need to be written every time

	if (m_flowField->IsInBounds(tileCoords))
	{
		return m_flowField->GetValueAtCoords(tileCoords);
	}

	// Out of Bounds: Has default value
	IntVec2 dimensions = m_flowField->m_dimensions;

	if (tileCoords.x == -1 && tileCoords.y == -1)
	{
		return Vec2(1.f, 1.f).GetNormalized();
	}
	if (tileCoords.x == -1 && tileCoords.y == dimensions.y)
	{
		return Vec2(1.f, -1.f).GetNormalized();
	}
	if (tileCoords.x == dimensions.x && tileCoords.y == -1)
	{
		return Vec2(-1.f, 1.f).GetNormalized();
	}
	if (tileCoords.x == dimensions.x && tileCoords.y == dimensions.y)
	{
		return Vec2(-1.f, -1.f).GetNormalized();
	}

	if (tileCoords.x == -1 && (tileCoords.y >= 0) && (tileCoords.y < dimensions.y))
	{
		return Vec2(1.f, 0.f);
	}
	if (tileCoords.x == dimensions.x && (tileCoords.y >= 0) && (tileCoords.y < dimensions.y))
	{
		return Vec2(-1.f, 0.f);
	}
	if (tileCoords.y == -1 && (tileCoords.x >= 0) && (tileCoords.x < dimensions.x))
	{
		return Vec2(0.f, 1.f);
	}
	if (tileCoords.y == dimensions.y && (tileCoords.x >= 0) && (tileCoords.x < dimensions.x))
	{
		return Vec2(0.f, -1.f);
	}

	return Vec2::ZERO;
}

void Map::Render() const
{
	RenderStatics();
	RenderActors();
	RenderDebug();
}



void Map::RenderStatics() const
{
	// Render Map
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, static_cast<unsigned int>(m_indexes.size()));
}

void Map::RenderActors() const
{
	for (Actor* actor : m_allActors)
	{
		if (actor != nullptr)
		{
			actor->Render();
		}
	}
}

void Map::RenderDebug() const
{
	if (!g_isDebugDraw)
	{
		return;
	}

	//DrawReachableMap();
	DrawExposureMap();
	DrawFlowField();
}

void Map::DrawReachableMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y));

	m_reachableMap->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, dimensions), FloatRange(0.f, UNREACHABLE_VALUE), Rgba8::TRANSPARENT_BLACK, Rgba8::MAGENTA);

	g_theRenderer->SetModelConstants(Mat44::MakeTranslation3D(Vec3(0.f, 0.f, 0.01f)));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Map::DrawExposureMap() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y));

	m_exposureMap->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, dimensions), m_exposureMap->GetRangeOffValuesExcludingSpecial(SPECIAL_VALUE_POS), 0.5f,
		Rgba8(0, 233, 233), Rgba8(0, 0, 255), Rgba8(70, 0, 0), Rgba8(255, 0, 0), SPECIAL_VALUE_POS, Rgba8::YELLOW);

	g_theRenderer->SetModelConstants(Mat44::MakeTranslation3D(Vec3(0.f, 0.f, 0.02f)));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Map::DrawFlowField() const
{
	std::vector<Vertex_PCU> verts;

	Vec2 dimensions = Vec2(static_cast<float>(m_dimensions.x), static_cast<float>(m_dimensions.y));

	m_flowField->AddVertsForDebugDraw(verts, AABB2(Vec2::ZERO, dimensions), 0.1f, Rgba8::GREEN);

	g_theRenderer->SetModelConstants(Mat44::MakeTranslation3D(Vec3(0.f, 0.f, 0.021f)));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	g_theRenderer->DrawVertexArray(verts);
}

void Map::RenderScreen() const
{
	RenderScreenDebug();
}

void Map::RenderScreenDebug() const
{
	if (!g_isDebugDraw)
	{
		return;
	}

	//Camera const& currentScreenCamera = g_theGame->m_currentRenderingPlayer->m_screenCamera;
	//AABB2 cameraBounds = AABB2(currentScreenCamera.GetOrthographicBottomLeft(), currentScreenCamera.GetOrthographicTopRight());
}

RaycastResultWithActor Map::RaycastAll(Vec3 const& start, Vec3 const& direction, float distance, Actor* owner /*= nullptr*/) const
{
	RaycastResultWithActor raycastAllResult;
	raycastAllResult.m_rayStartPos = start;
	raycastAllResult.m_rayFwdNormal = direction;
	raycastAllResult.m_rayLength = distance;


	RaycastResultWithActor result = RaycastWorldXY(start, direction, distance);
	if (result.m_didImpact)
	{
		if (!raycastAllResult.m_didImpact)
		{
			raycastAllResult = result;
		}
		else if (raycastAllResult.m_impactDist > result.m_impactDist)
		{
			raycastAllResult = result;
		}
	}



	result = RaycastWorldZ(start, direction, distance);
	if (result.m_didImpact)
	{
		if (!raycastAllResult.m_didImpact)
		{
			raycastAllResult = result;
		}
		else if (raycastAllResult.m_impactDist > result.m_impactDist)
		{
			raycastAllResult = result;
		}
	}


	result = RaycastWorldActors(start, direction, distance, owner);
	if (result.m_didImpact)
	{
		if (!raycastAllResult.m_didImpact)
		{
			raycastAllResult = result;
		}
		else if (raycastAllResult.m_impactDist > result.m_impactDist)
		{
			raycastAllResult = result;
		}
	}

	return raycastAllResult;
}

RaycastResultWithActor Map::RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const
{
	RaycastResultWithActor raycastResult;
	raycastResult.m_rayStartPos = start;
	raycastResult.m_rayFwdNormal = direction;
	raycastResult.m_rayLength = distance;

	int tileX = RoundDownToInt(start.x);
	int tileY = RoundDownToInt(start.y);

	if (IsTileSolid(tileX, tileY) && IsPositionInBounds(start))
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = start;
		raycastResult.m_impactNormal = -direction;
		return raycastResult;
	}

	float fwdDistPerXCrossing = 1.0f / fabsf(direction.x); // 1 / cos theta
	int tileStepDirectionX = (direction.x < 0.f) ? -1 : 1;
	float xAtFirstXCrossing = static_cast<float>(tileX) + static_cast<float>(tileStepDirectionX + 1) * 0.5f;
	float xDistToFirstXCrossing = xAtFirstXCrossing - start.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistPerXCrossing; // forward direction distance


	float fwdDistPerYCrossing = 1.0f / fabsf(direction.y); // 1 / sin theta
	int tileStepDirectionY = (direction.y < 0.f) ? -1 : 1;
	float yAtFirstYCrossing = static_cast<float>(tileY) + static_cast<float>(tileStepDirectionY + 1) * 0.5f;
	float yDistToFirstYCrossing = yAtFirstYCrossing - start.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistPerYCrossing; // forward direction distance

	// TODO Stop iterating when are outside of the tile grid
	while (true)
	{
		if (fwdDistAtNextXCrossing <= fwdDistAtNextYCrossing)
		{
			if (fwdDistAtNextXCrossing > distance)
			{
				raycastResult.m_impactDist = distance;
				return raycastResult;
			}
			tileX += tileStepDirectionX;

			Vec3 currentPos = start + fwdDistAtNextXCrossing * direction;
			if (IsPositionInBounds(currentPos))
			{
				if (IsTileSolid(tileX, tileY))
				{
					raycastResult.m_didImpact = true;
					raycastResult.m_impactDist = fwdDistAtNextXCrossing;
					raycastResult.m_impactPos = start + raycastResult.m_impactDist * direction;
					raycastResult.m_impactNormal = Vec3(-static_cast<float>(tileStepDirectionX), 0.f, 0.f);
					return raycastResult;
				}
			}
			fwdDistAtNextXCrossing += fwdDistPerXCrossing;
		}
		else
		{
			if (fwdDistAtNextYCrossing > distance)
			{
				raycastResult.m_impactDist = distance;
				return raycastResult;
			}
			tileY += tileStepDirectionY;

			Vec3 currentPos = start + fwdDistAtNextYCrossing * direction;
			if (IsPositionInBounds(currentPos))
			{
				if (IsTileSolid(tileX, tileY))
				{
					raycastResult.m_didImpact = true;
					raycastResult.m_impactDist = fwdDistAtNextYCrossing;
					raycastResult.m_impactPos = start + raycastResult.m_impactDist * direction;
					raycastResult.m_impactNormal = Vec3(0.f, -static_cast<float>(tileStepDirectionY), 0.f);
					return raycastResult;
				}
			}
			fwdDistAtNextYCrossing += fwdDistPerYCrossing;
		}
	}

	/*return raycastResult;*/
}

RaycastResultWithActor Map::RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const
{
	float constexpr CEILINGZ = 1.f;
	float constexpr FLOORZ = 0.f;

	RaycastResultWithActor raycastResult;
	raycastResult.m_rayStartPos = start;
	raycastResult.m_rayFwdNormal = direction;
	raycastResult.m_rayLength = distance;

	if (direction.z > 0.f)
	{
		float t = (CEILINGZ - start.z) / (direction.z * distance);
		if (t < 0.f || t >= 1.f)
		{
			return raycastResult;
		}

		Vec3 impactPos = start + t * distance * direction;
		if (!IsPositionInBounds(impactPos))
		{
			return raycastResult;
		}

		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = t * distance;
		raycastResult.m_impactPos = start + raycastResult.m_impactDist * direction;
		raycastResult.m_impactNormal = Vec3(0.f, 0.f, -1.f);
		return raycastResult;

	}
	else if (direction.z < 0.f)
	{
		float t = (FLOORZ - start.z) / (direction.z * distance);
		if (t < 0.f || t >= 1.f)
		{
			return raycastResult;
		}

		Vec3 impactPos = start + t * distance * direction;
		if (!IsPositionInBounds(impactPos))
		{
			return raycastResult;
		}

		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = t * distance;
		raycastResult.m_impactPos = start + raycastResult.m_impactDist * direction;
		raycastResult.m_impactNormal = Vec3(0.f, 0.f, 1.f);
		return raycastResult;
	}

	return raycastResult;
}

RaycastResultWithActor Map::RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance, Actor* owner /*= nullptr*/) const
{
	RaycastResultWithActor raycastResult;
	raycastResult.m_rayStartPos = start;
	raycastResult.m_rayFwdNormal = direction;
	raycastResult.m_rayLength = distance;

	int numActor = (int)m_allActors.size();
	for (int actorIndex = 0; actorIndex < numActor; ++actorIndex)
	{
		Actor* actor = m_allActors[actorIndex];
		// Self and Dead
		if (owner == actor || !IsAlive(actor))
		{
			continue;
		}
		// Your projectile
		if (actor->m_owner == owner->m_handle)
		{
			continue;
		}
		// Ignore those without collision
		if (!actor->m_definition->m_collision.m_collidesWithActors && !actor->m_definition->m_collision.m_collidesWithWorld)
		{
			continue;
		}

		Vec2 cylinderCenterXY = Vec2(actor->m_position.x, actor->m_position.y);
		float cylinderRadius = actor->m_definition->m_collision.m_physicsRadius;
		FloatRange cylinderMinMaxZ = FloatRange(actor->m_position.z , actor->m_position.z + actor->m_definition->m_collision.m_physicsHeight);


		RaycastResult3D result = RaycastVsCylinderZ3D(start, direction, distance, cylinderCenterXY, cylinderMinMaxZ, cylinderRadius);
		if (result.m_didImpact)
		{
			if (!raycastResult.m_didImpact)
			{
				raycastResult.m_didImpact = result.m_didImpact;
				raycastResult.m_impactDist = result.m_impactDist;
				raycastResult.m_impactPos = result.m_impactPos;
				raycastResult.m_impactNormal = result.m_impactNormal;

				raycastResult.m_hitActor = actor;
			}
			else if (raycastResult.m_impactDist > result.m_impactDist)
			{
				raycastResult.m_didImpact = result.m_didImpact;
				raycastResult.m_impactDist = result.m_impactDist;
				raycastResult.m_impactPos = result.m_impactPos;
				raycastResult.m_impactNormal = result.m_impactNormal;

				raycastResult.m_hitActor = actor;
			}
		}
	}

	return raycastResult;
}

void Map::DeleteDestroyedActors()
{
	for (int actorIndex = 0; actorIndex < (int)m_allActors.size(); ++actorIndex)
	{
		Actor* actor = m_allActors[actorIndex];
		if (actor != nullptr && actor->m_isGarbage)
		{
			delete actor;
			m_allActors[actorIndex] = nullptr;
		}
	}
}

Actor* Map::SpawnActor(SpawnInfo const& spawnInfo)
{
	if (m_currentUid >= ActorHandle::MAX_ACTOR_UID || m_allActors.size() >= ActorHandle::MAX_ACTOR_INDEX)
	{
		ERROR_AND_DIE("Num of Actors reaches maximum.");
	}
	// find empty slot
	for (int actorIndex = 0; actorIndex < (int)m_allActors.size(); ++actorIndex)
	{
		if (m_allActors[actorIndex] == nullptr)
		{
			Actor* newActor = new Actor(this, spawnInfo, ActorHandle(m_currentUid, actorIndex));
			m_allActors[actorIndex] = newActor;
			m_currentUid++;
			return newActor;
		}
	}
	// no empty slot
	Actor* newActor = new Actor(this, spawnInfo, ActorHandle(m_currentUid, static_cast<unsigned int>(m_allActors.size())));
	m_allActors.push_back(newActor);
	m_currentUid++;
	return newActor;
}

void Map::SpawnPlayer(int playerIndex)
{
	int numSpawnPoint = (int)m_spawnPoints.size();
	if (numSpawnPoint == 0)
	{
		ERROR_AND_DIE("No spawn points in the map.");
	}
	int randomIndex = g_rng.RollRandomIntLessThan(numSpawnPoint);
	SpawnInfo info;
	info.m_actor = "Marine";
	info.m_position = m_spawnPoints[randomIndex].m_position;
	info.m_orientation = m_spawnPoints[randomIndex].m_orientation;
	info.m_velocity = m_spawnPoints[randomIndex].m_velocity;
	Actor* newMarine = SpawnActor(info);

	g_theGame->m_players[playerIndex]->Possess(newMarine);
}

Actor* Map::GetActorByHandle(ActorHandle const& handle) const
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	Actor* actor = m_allActors[handle.GetIndex()];
	if (actor != nullptr && actor->m_handle == handle)
	{
		return actor;
	}
	else
	{
		return nullptr;
	}
}

Actor* Map::GetClosestVisibleEnemy(Actor* self)
{
	Actor* closestActor = nullptr;
	float closestDistanceSquared = 100000000.f;

	int numActor = (int)m_allActors.size();
	for (int actorIndex = 0; actorIndex < numActor; ++actorIndex)
	{
		Actor* actor = m_allActors[actorIndex];
		if (!IsAlive(actor))
		{
			continue;
		}
		if (!self->IsOpposingFaction(actor->m_definition->m_faction))
		{
			continue;
		}
		if (IsPointInsideDirectedSector2D(Vec2(actor->m_position.x, actor->m_position.y), Vec2(self->m_position.x, self->m_position.y), self->GetForwardNormal2D(), self->m_definition->m_ai.m_sightAngle, self->m_definition->m_ai.m_sightRadius))
		{
			Vec3 disp = (actor->m_position - self->m_position);
			Vec3 startPos = self->GetEyePosition();
			Vec3 direction = disp.GetNormalized();
			float distance = self->m_definition->m_ai.m_sightRadius;

			RaycastResultWithActor result = RaycastWorldXY(startPos, direction, distance);
			float distanceXYSquared = Vec2(disp.x, disp.y).GetLengthSquared();
			if (!result.m_didImpact || result.m_impactDist * result.m_impactDist > distanceXYSquared)
			{
				if (distanceXYSquared < closestDistanceSquared)
				{
					closestActor = actor;
					closestDistanceSquared = distanceXYSquared;
				}
			}
		}
		
	}

	return closestActor;
}

void Map::DebugPossessNext(Player* playerController)
{
	if (playerController == nullptr)
	{
		return;
	}

	int originalActorIndex = playerController->m_actorHandle.GetIndex();

	int numActor = (int)m_allActors.size();

	for (int offset = 1; offset <= numActor; ++offset)
	{
		int newActorIndex = (originalActorIndex + offset) % numActor;
		Actor* newActor = m_allActors[newActorIndex];
		if (newActor != nullptr)
		{
			if (newActor->m_definition->m_canBePossessed)
			{
				playerController->Possess(newActor);
				return;
			}
		}
	}

	DebugAddMessage("There is no actors which can be possessed in the map!", 5.f, Rgba8::RED, Rgba8::RED);
}

void Map::OnPlayerDie()
{
	//SpawnPlayer(); // Move it to Map::Update()
}

bool Map::IsAlive(Actor const* actor) const
{
	return (actor != nullptr && !actor->m_isDead);
}

bool Map::IsGameFinished() const
{
	for (Actor* actor : m_allActors)
	{
		if (actor != nullptr && actor->m_definition->m_faction == Faction::DEMON)
		{
			return false;
		}
	}
	return true;
}

void Map::ShowNumOfAliveDemons() const
{
	int numOfDemons = 0;
	for (Actor* actor : m_allActors)
	{
		if (actor != nullptr && actor->m_definition->m_faction == Faction::DEMON && !actor->m_isDead)
		{
			numOfDemons++;
		}
	}
	
	DebugAddMessage(Stringf("%d Demons Alive", numOfDemons), 4.f, Rgba8::RED);
}

//-----------------------------------------------------------------------------------------------
void Map::CreateTiles()
{
	m_dimensions = m_definition->m_image.GetDimensions();
	m_tiles.resize(m_dimensions.x * m_dimensions.y);
	m_vertexes.reserve(m_dimensions.x * m_dimensions.y * 16);
	m_indexes.reserve(m_dimensions.x * m_dimensions.y * 24);

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + tileY * m_dimensions.x;
			Rgba8 mapColor = m_definition->m_image.GetTexelColor(IntVec2(tileX, tileY));
			TileDefinition const* tileDef = TileDefinition::GetByMapColor(mapColor);
			m_tiles[tileIndex].m_bounds = AABB3(static_cast<float>(tileX), static_cast<float>(tileY), 0.f,
												static_cast<float>(tileX + 1), static_cast<float>(tileY + 1), 1.f);
			m_tiles[tileIndex].m_tileDef = tileDef;
		}
	}
}

void Map::CreateNavGrids()
{
	TileHeatMap floodMap = TileHeatMap(m_dimensions, SPECIAL_VALUE_POS);
	Vec3 firstSpawnPoint = m_spawnPoints[0].m_position; // Need to be after having spawn points
	floodMap.SetValueAtCoords(GetCoordsForWorldPos(firstSpawnPoint.x, firstSpawnPoint.y), 0.f);
	SpreadDistanceMapHeat(floodMap, 0.f);

	m_reachableMap = new TileHeatMap(m_dimensions);
	// Fill Unreachable Tiles
	int numTiles = m_reachableMap->GetNumTiles();
	for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex)
	{
		if (floodMap.GetValueAtIndex(tileIndex) == SPECIAL_VALUE_POS)
		{
			m_reachableMap->SetValueAtIndex(tileIndex, UNREACHABLE_VALUE);
		}
	}

	m_exposureMap = new TileHeatMap(m_dimensions);
	m_flowField = new TileVectorField(m_dimensions, Vec2::ZERO);
}

void Map::CreateGeometry()
{
	m_spriteSheet = new SpriteSheet(*m_definition->m_spriteSheetTexture, m_definition->m_spriteSheetCellCount);

	int const spriteSheetX = m_definition->m_spriteSheetCellCount.x;

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + tileY * m_dimensions.x;
			TileDefinition const* tileDef = m_tiles[tileIndex].m_tileDef;

			IntVec2 wallSpriteCoords = tileDef->m_wallSpriteCoords;
			if (wallSpriteCoords != IntVec2(-1, -1))
			{
				AABB2 wallUVs = m_spriteSheet->GetSpriteUVs(wallSpriteCoords.x + wallSpriteCoords.y * spriteSheetX);
				AddGeometryForWall(m_tiles[tileIndex].m_bounds, wallUVs);
			}
			

			IntVec2 floorSpriteCoords = tileDef->m_floorSpriteCoords;
			if (floorSpriteCoords != IntVec2(-1, -1))
			{
				AABB2 floorUVs = m_spriteSheet->GetSpriteUVs(floorSpriteCoords.x + floorSpriteCoords.y * spriteSheetX);
				AddGeometryForFloor(m_tiles[tileIndex].m_bounds, floorUVs);
			}
			

			IntVec2 ceilingSpriteCoords = tileDef->m_ceilingSpriteCoords;
			if (ceilingSpriteCoords != IntVec2(-1, -1))
			{
				AABB2 ceilingUVs = m_spriteSheet->GetSpriteUVs(ceilingSpriteCoords.x + ceilingSpriteCoords.y * spriteSheetX);
				AddGeometryForCeiling(m_tiles[tileIndex].m_bounds, ceilingUVs);
			}
			
		}
	}
}

void Map::AddGeometryForWall(AABB3 const& bounds, AABB2 const& UVs)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float minZ = bounds.m_mins.z;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;
	float maxZ = bounds.m_maxs.z;

	// p-max n-min
	Vec3 nnn(minX, minY, minZ);
	Vec3 nnp(minX, minY, maxZ);
	Vec3 npn(minX, maxY, minZ);
	Vec3 npp(minX, maxY, maxZ);
	Vec3 pnn(maxX, minY, minZ);
	Vec3 pnp(maxX, minY, maxZ);
	Vec3 ppn(maxX, maxY, minZ);
	Vec3 ppp(maxX, maxY, maxZ);

	AddVertsForWalls(m_vertexes, m_indexes, pnn, ppn, ppp, pnp, Rgba8::OPAQUE_WHITE, UVs);
	AddVertsForWalls(m_vertexes, m_indexes, npn, nnn, nnp, npp, Rgba8::OPAQUE_WHITE, UVs);
	AddVertsForWalls(m_vertexes, m_indexes, ppn, npn, npp, ppp, Rgba8::OPAQUE_WHITE, UVs);
	AddVertsForWalls(m_vertexes, m_indexes, nnn, pnn, pnp, nnp, Rgba8::OPAQUE_WHITE, UVs);
}

void Map::AddGeometryForFloor(AABB3 const& bounds, AABB2 const& UVs)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float minZ = bounds.m_mins.z;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;

	// p-max n-min
	Vec3 nnn(minX, minY, minZ);
	Vec3 npn(minX, maxY, minZ);
	Vec3 pnn(maxX, minY, minZ);
	Vec3 ppn(maxX, maxY, minZ);


	AddVertsForWalls(m_vertexes, m_indexes, nnn, pnn, ppn, npn, Rgba8::OPAQUE_WHITE, UVs);
}

void Map::AddGeometryForCeiling(AABB3 const& bounds, AABB2 const& UVs)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;
	float maxZ = bounds.m_maxs.z;

	// p-max n-min
	Vec3 nnp(minX, minY, maxZ);
	Vec3 npp(minX, maxY, maxZ);
	Vec3 pnp(maxX, minY, maxZ);
	Vec3 ppp(maxX, maxY, maxZ);

	AddVertsForWalls(m_vertexes, m_indexes, npp, ppp, pnp, nnp, Rgba8::OPAQUE_WHITE, UVs);
}

void Map::CreateBuffers()
{
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(1 * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	g_theRenderer->CopyCPUToGPU(m_vertexes.data(), static_cast<unsigned int>(m_vertexes.size()) * m_vertexBuffer->GetStride(), m_vertexBuffer);

	m_indexBuffer = g_theRenderer->CreateIndexBuffer(1 * sizeof(unsigned int));
	g_theRenderer->CopyCPUToGPU(m_indexes.data(), static_cast<unsigned int>(m_indexes.size()) * m_indexBuffer->GetStride(), m_indexBuffer);
}

void Map::CreateSpawnPoints()
{
	for (SpawnInfo info : m_definition->m_spawnInfos)
	{
		if (info.m_actor == "SpawnPoint")
		{
			m_spawnPoints.push_back(info);
		}
	}
}

void Map::SpawnNonPlayerActors()
{
	for (SpawnInfo info : m_definition->m_spawnInfos)
	{
		if (info.m_actor != "SpawnPoint" && info.m_actor != "Marine")
		{
			SpawnActor(info);
		}
	}
}

bool Map::IsPositionInBounds(Vec3 const& position) const
{
	Vec3 small = Vec3(0.0001f, 0.0001f, 0.0001f);
	AABB3 mapBounds = AABB3(Vec3::ZERO - small, Vec3((float)m_dimensions.x, (float)m_dimensions.y, 1.f) + small);
	return mapBounds.IsPointInside(position);
}

bool Map::AreCoordsInBounds(int x, int y) const
{
	return (x >= 0) && (y >= 0) && (x <= m_dimensions.x) && (y <= m_dimensions.y);
}

Tile const* Map::GetTile(int x, int y) const
{
	GUARANTEE_OR_DIE(AreCoordsInBounds(x, y), "Try to get tile which is out of bounds.");
	int tileIndex = x + y * m_dimensions.x;
	return &m_tiles[tileIndex];
}

IntVec2 Map::GetCoordsForWorldPos(float x, float y) const
{
	int tileX = RoundDownToInt(x);
	int tileY = RoundDownToInt(y);
	return IntVec2(tileX, tileY);
}

bool Map::IsTileSolid(int x, int y) const
{
	if (!AreCoordsInBounds(x, y))
	{
		return true;
	}
	Tile const* tile = GetTile(x, y);
	return tile->m_tileDef->m_isSolid;
}

Vec2 Map::GetTileCenter(int tileX, int tileY) const
{
	return Vec2(static_cast<float>(tileX) + 0.5f, static_cast<float>(tileY) + 0.5f);
}

