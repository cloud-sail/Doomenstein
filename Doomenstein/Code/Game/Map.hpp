#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Tile.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

class TileHeatMap;
class TileVectorField;
struct RaycastResult2D;

class Map
{
public:
	Map(Game* game, const MapDefinition* definition);
	~Map();

	void CreateTiles();
	void CreateNavGrids();

	void CreateGeometry();
	void AddGeometryForWall(AABB3 const& bounds, AABB2 const& UVs);
	void AddGeometryForFloor(AABB3 const& bounds, AABB2 const& UVs);
	void AddGeometryForCeiling(AABB3 const& bounds, AABB2 const& UVs);
	void CreateBuffers();
	void CreateSpawnPoints();
	void SpawnNonPlayerActors();

	bool IsPositionInBounds(Vec3 const& position) const;
	bool AreCoordsInBounds(int x, int y) const;
	Tile const* GetTile(int x, int y) const;
	IntVec2 GetCoordsForWorldPos(float x, float y) const;
	bool IsTileSolid(int x, int y) const;
	Vec2 GetTileCenter(int tileX, int tileY) const;

	void Update();
	void UpdateActors();
	void CollideActors();
	void CollideActor(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorWithMap(Actor* actor);
	void CheckAndSpawnPlayers();

	void UpdateNavGrids();
	void UpdateExposureMap();
	void UpdateFlowField();
	RaycastResult2D FastVoxelRaycast(Vec2 rayStart, Vec2 rayForwardNormal, float rayLength) const;
	void SpreadDistanceMapHeat(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep = 1.f);
	void SpreadDistanceMapHeatOnReachableMap(TileHeatMap& distanceMap, float startSearchValue, float heatSpreadStep = 1.f);
	bool IsTileUnreachable(int tileX, int tileY) const;
	Vec2 GetBilinearInterpResultFromWorldPos(Vec2 const& worldPos) const;
	Vec2 GetSafeValueFromFlowField(IntVec2 tileCoords) const;

	void Render() const;
	void RenderStatics() const;
	void RenderActors() const;
	void RenderDebug() const;
	void DrawReachableMap() const;
	void DrawExposureMap() const;
	void DrawFlowField() const;

	void RenderScreen() const;
	void RenderScreenDebug() const;

	RaycastResultWithActor RaycastAll(Vec3 const& start, Vec3 const& direction, float distance, Actor* owner = nullptr) const; // ignore self?
	RaycastResultWithActor RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const; 
	RaycastResultWithActor RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResultWithActor RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance, Actor* owner = nullptr) const;

	void DeleteDestroyedActors();
	Actor* SpawnActor(SpawnInfo const& spawnInfo);
	void SpawnPlayer(int playerIndex);
	Actor* GetActorByHandle(ActorHandle const& handle) const;
	Actor* GetClosestVisibleEnemy(Actor* self); // AI search opposite faction actor,using its
	
	void DebugPossessNext(Player* playerController);
	void OnPlayerDie();

	bool IsAlive(Actor const* actor) const;
	bool IsGameFinished() const;
	void ShowNumOfAliveDemons() const;

public:
	Game* m_game = nullptr; // g_theGame
	std::vector<Actor*> m_allActors;


protected:
	unsigned int m_currentUid = 0;

	std::vector<SpawnInfo> m_spawnPoints;

	MapDefinition const* m_definition = nullptr;
	std::vector<Tile> m_tiles;
	IntVec2 m_dimensions;

	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;
	Texture const* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr; // Remember to delete it
	IndexBuffer* m_indexBuffer = nullptr; // Remember to delete it

	SpriteSheet* m_spriteSheet = nullptr;

	TileHeatMap* m_reachableMap = nullptr; // represent the place actor can reach, not is solid
	TileHeatMap* m_exposureMap = nullptr;
	TileVectorField* m_flowField = nullptr;
};

