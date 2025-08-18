#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec3.hpp"
//-----------------------------------------------------------------------------------------------
class	AudioSystem;
class	InputSystem;
class	Renderer;
class	Window;
class	App;
class	Clock;
class	Shader;
class	Texture;
class	VertexBuffer;
class	IndexBuffer;
class	SpriteSheet;
class	Skybox;

struct	Vertex_PCU;
struct	Vertex_PCUTBN;
struct	IntVec2;
struct	AABB2;
struct	AABB3;
struct	Vec3;
struct	RaycastResult3D;
//-----------------------------------------------------------------------------------------------
class	Game;
class	Player;
class	Actor;
class	Map;
class	Controller;
class	RandomNumberGenerator;
struct	MapDefinition;
struct	TileDefinition;
struct	ActorDefinition;
struct	WeaponDefinition;
struct	Tile;
struct	SpawnInfo;
struct	ActorHandle;
//-----------------------------------------------------------------------------------------------
extern AudioSystem*		g_theAudio;
extern InputSystem*		g_theInput;
extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern App*				g_theApp;
extern Game*			g_theGame;
extern RandomNumberGenerator g_rng;

//-----------------------------------------------------------------------------------------------
extern bool g_isDebugDraw;




//-----------------------------------------------------------------------------------------------
// Gameplay Constants
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;

constexpr float CAMERA_MOVE_SPEED = 1.f;
constexpr float CAMERA_YAW_TURN_RATE = 270.f;
constexpr float CAMERA_PITCH_TURN_RATE = 90.f;
constexpr float CAMERA_ROLL_TURN_RATE = 90.f;
constexpr float CAMERA_SPEED_FACTOR = 15.f;

constexpr float CAMERA_MAX_PITCH = 85.f;
constexpr float CAMERA_MAX_ROLL = 45.f;

constexpr float MOUSE_AIM_RATE = 0.075f;

constexpr float CAMERA_ZNEAR = 0.1f;
constexpr float CAMERA_ZFAR = 100.f;

constexpr float SWITCH_WEAPON_TIME = 1.f;

constexpr float MAX_WEAPON_SHAKE_RADIUS_RATIO = 0.2f;
constexpr float MAX_WEAPON_SHAKE_DEGREES = 70.f;

constexpr float MIN_SPECIAL_MUSIC_VOLUME = 0.3f;
constexpr float MAX_SPECIAL_MUSIC_VOLUME = 0.6f;

//-----------------------------------------------------------------------------------------------
enum GameInputDevice
{
	KEYBOARD_AND_MOUSE = -1,
	GAMEPAD_0,
};



struct RaycastResultWithActor
{
	Vec3	m_rayStartPos;
	Vec3	m_rayFwdNormal;
	float	m_rayLength = 1.f;

	bool	m_didImpact = false;
	float	m_impactDist = 0.0f;
	Vec3	m_impactPos;
	Vec3	m_impactNormal;

	Actor*	m_hitActor = nullptr;
};



void AddVertsForWalls(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void DebugDrawRaycastResult3D(RaycastResultWithActor result);



