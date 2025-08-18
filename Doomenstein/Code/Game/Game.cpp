#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "Game/Sound.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Skybox.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Window/Window.hpp"

//-----------------------------------------------------------------------------------------------
static SoundID	MAIN_MENU_MUSIC = MISSING_SOUND_ID;
static SoundID	GAME_MUSIC = MISSING_SOUND_ID;
static SoundID	VICTORY_MUSIC = MISSING_SOUND_ID;
static SoundID	BUTTOM_CLICK_SOUND = MISSING_SOUND_ID;
static SoundID	SPECIAL_WEAPON_MUSIC = MISSING_SOUND_ID;
static float	MUSIC_VOLUME = 0.1f;


//-----------------------------------------------------------------------------------------------
static char const* ATTRACT_SCREEN_TEXT = \
"Press SPACE to join with mouse and keyboard\n\
Press START to join with controller\n\
Press ESCAPE or BACK to exit";


//-----------------------------------------------------------------------------------------------
Game::Game()
{
	TileDefinition::InitializeDefinitions();
	MapDefinition::InitializeDefinitions();
	WeaponDefinition::InitializeDefinitions();
	ActorDefinition::InitializeDefinitions();
	LoadAudio();
	ResetLighting();


	m_clock = new Clock();

	SkyboxConfig skyboxConfig;
	skyboxConfig.m_defaultRenderer = g_theRenderer;
	skyboxConfig.m_fileName = "Data/Images/skybox_night.png";
	skyboxConfig.m_type = SkyboxType::CUBE_MAP;
	m_skybox = new Skybox(skyboxConfig);

	SetNextState(GameState::ATTRACT);
}

Game::~Game()
{
	StopMusic();
	DestroyPlayers();
	DestroyMaps();
	delete m_clock;
}

void Game::Update()
{
	UpdateDeveloperCheats();

	SwitchToNextState();
	UpdateCurrentState();
}

void Game::Render() const
{
	RenderCurrentState();
}

Map* Game::GetCurrentMap() const
{
	return m_currentMap;
}

bool Game::IsMultiplayerMode() const
{
	return m_players.size() > 1;
}

float Game::GetPlayerCameraAspect() const
{
	float aspect = Window::s_mainWindow->GetAspectRatio();
	return (IsMultiplayerMode()) ? aspect * 2.0f : aspect;
}

void Game::UpdateDeveloperCheats()
{
	AdjustForPauseAndTimeDistortion();
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		g_isDebugDraw = !g_isDebugDraw;
	}
}

void Game::AdjustForPauseAndTimeDistortion()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_P))
	{
		m_clock->TogglePause();
	}

	if (g_theInput->IsKeyDown(KEYCODE_O))
	{
		m_clock->StepSingleFrame();
	}
	
	bool isSlowMo = g_theInput->IsKeyDown(KEYCODE_T);

	m_clock->SetTimeScale(isSlowMo ? 0.1 : 1.0);
}

//-----------------------------------------------------------------------------------------------
void Game::RenderUI() const
{
}

void Game::UpdateCameras()
{
	// Screen camera (for UI, HUD, Attract, etc.)
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::CreatePlayers()
{
	GUARANTEE_OR_DIE(m_players.empty(), "Remaining players!");
	GUARANTEE_OR_DIE(m_isPlayer1Taken || m_isPlayer2Taken, "Enter playing with no player!");

	if (m_isPlayer1Taken && m_isPlayer2Taken)
	{
		Player* player1 = new Player();
		player1->m_playerIndex = 0;
		player1->m_controllerIndex = (m_isPlayer1UsingKeyBoard) ? KEYBOARD_AND_MOUSE : GAMEPAD_0;
		player1->m_camera.SetNormalizedViewPort(AABB2(0.f, 0.5f, 1.f, 1.f));
		player1->m_screenCamera.SetNormalizedViewPort(AABB2(0.f, 0.5f, 1.f, 1.f));
		player1->m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y * 0.5f));
		m_players.push_back(player1);

		Player* player2 = new Player();
		player2->m_playerIndex = 1;
		player2->m_controllerIndex = (!m_isPlayer1UsingKeyBoard) ? KEYBOARD_AND_MOUSE : GAMEPAD_0;
		player2->m_camera.SetNormalizedViewPort(AABB2(0.f, 0.f, 1.f, 0.5f));
		player2->m_screenCamera.SetNormalizedViewPort(AABB2(0.f, 0.f, 1.f, 0.5f));
		player2->m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y * 0.5f));
		m_players.push_back(player2);

		g_theAudio->SetNumListeners(2);
	}
	else if (m_isPlayer1Taken)
	{
		// Player 1
		Player* player1 = new Player();
		player1->m_playerIndex = 0;
		player1->m_controllerIndex = (m_isPlayer1UsingKeyBoard) ? KEYBOARD_AND_MOUSE : GAMEPAD_0;
		player1->m_camera.SetNormalizedViewPort(AABB2::ZERO_TO_ONE);
		player1->m_screenCamera.SetNormalizedViewPort(AABB2::ZERO_TO_ONE);
		player1->m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_players.push_back(player1);

		g_theAudio->SetNumListeners(1);
	}
	else if (m_isPlayer2Taken)
	{
		Player* player2 = new Player();
		player2->m_playerIndex = 0;
		player2->m_controllerIndex = (!m_isPlayer1UsingKeyBoard) ? KEYBOARD_AND_MOUSE : GAMEPAD_0;
		player2->m_camera.SetNormalizedViewPort(AABB2::ZERO_TO_ONE);
		player2->m_screenCamera.SetNormalizedViewPort(AABB2::ZERO_TO_ONE);
		player2->m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_players.push_back(player2);

		g_theAudio->SetNumListeners(1);
	}
}

void Game::DestroyPlayers()
{
	for (Player* player : m_players) {
		delete player;
	}
	m_players.clear();
	m_currentRenderingPlayer = nullptr;
}

void Game::CreateMaps()
{
	std::string defaultMap = g_gameConfigBlackboard.GetValue("defaultMap", "TestMap");
	TrimSpace(defaultMap);
	MapDefinition const* mapDef = MapDefinition::GetByName(defaultMap);
	m_currentMap = new Map(this, mapDef);
}


void Game::DestroyMaps()
{
	delete m_currentMap;
	m_currentMap = nullptr;
}

void Game::DebugDrawUpdate()
{
	// Game Clock Data
	float totalSeconds = (float)m_clock->GetTotalSeconds();
	float frameRate = (float)m_clock->GetFrameRate();
	float timeScale = (float)m_clock->GetTimeScale();

	AABB2 gameClockBox = AABB2(Vec2(SCREEN_SIZE_X * 0.6f, SCREEN_SIZE_Y * 0.96f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	if (frameRate == 0.f)
	{
		DebugAddScreenText(Stringf("Time: %.2f FPS:      inf Scale: %.2f", totalSeconds, timeScale),
			gameClockBox, 15.f, Vec2(0.98f, 0.5f), 0.f, 0.7f);
	}
	else
	{
		DebugAddScreenText(Stringf("Time: %.2f FPS: %8.02f Scale: %.2f", totalSeconds, frameRate, timeScale),
			gameClockBox, 15.f, Vec2(0.98f, 0.5f), 0.f, 0.7f);
	}

	//AABB2 lightConstantsBox = AABB2(Vec2(SCREEN_SIZE_X * 0.6f, SCREEN_SIZE_Y * 0.85f), Vec2(SCREEN_SIZE_X * 0.99f, SCREEN_SIZE_Y * 0.95f));

	//DebugAddScreenText(Stringf("Sun Direction X: %.2f [F2/F3 to change]\nSun Direction Y: %.2f [F4/F5 to change]\nSun Intensity: %.2f [F6/F7 to change]\nAmbient Intensity: %.2f [F8/F9 to change]", 
	//	m_sunDirection.x, m_sunDirection.y, m_sunIntensity, m_ambientIntensity),
	//	lightConstantsBox, 15.f, Vec2(0.98f, 0.5f), 0.f, 0.7f);
}

void Game::LoadAudio()
{
	MAIN_MENU_MUSIC		= g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("mainMenuMusic", ""));
	GAME_MUSIC			= g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("gameMusic", ""));
	VICTORY_MUSIC		= g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("victoryMusic", ""));
	BUTTOM_CLICK_SOUND	= g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("buttonClickSound", ""));
	MUSIC_VOLUME		= g_gameConfigBlackboard.GetValue("musicVolume", MUSIC_VOLUME);
	SPECIAL_WEAPON_MUSIC = g_theAudio->CreateOrGetSound(g_gameConfigBlackboard.GetValue("specialWeaponMusic", ""));
	m_specialMusicPlaybackID = g_theAudio->StartSound(SPECIAL_WEAPON_MUSIC, true, MIN_SPECIAL_MUSIC_VOLUME, 0.f, 1.f, true);

	Sound::Init();
}

void Game::PlayMusic(SoundID soundID, float volume /*= 0.5f*/)
{
	if (g_theAudio->IsMusicPlayingOnChannel(soundID, m_musicPlaybackID))
	{
		return;
	}
	if (m_musicPlaybackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_musicPlaybackID);
	}
	m_musicPlaybackID = g_theAudio->StartSound(soundID, true, volume);
}

void Game::SetPlayBackSpeed(float playBackSpeed)
{
	if (m_musicPlaybackID == MISSING_SOUND_ID)
	{
		return;
	}
	g_theAudio->SetSoundPlaybackSpeed(m_musicPlaybackID, playBackSpeed);
}

void Game::StopMusic()
{
	if (m_musicPlaybackID != MISSING_SOUND_ID)
	{
		g_theAudio->StopSound(m_musicPlaybackID);
	}
}

void Game::ResumeSpecialMusic()
{
	g_theAudio->ResumeSoundPlayback(m_specialMusicPlaybackID);
}

void Game::PauseSpecialMusic()
{
	g_theAudio->PauseSoundPlayback(m_specialMusicPlaybackID);
}

void Game::ActivateSpecialMusicVolume()
{
	g_theAudio->SetSoundPlaybackVolume(m_specialMusicPlaybackID, MAX_SPECIAL_MUSIC_VOLUME);
}

void Game::DeactivateSpecialMusicVolume()
{
	g_theAudio->SetSoundPlaybackVolume(m_specialMusicPlaybackID, MIN_SPECIAL_MUSIC_VOLUME);
}

void Game::ResetLighting()
{
	m_sunDirection = Vec3(2.f, 9.f, -1.f);
	//m_sunIntensity = 0.35f;
	//m_ambientIntensity = 0.25f;
	m_sunIntensity = 0.4f;
	m_ambientIntensity = 0.35f;
	ApplyLighting();
}

void Game::ApplyLighting()
{
	g_theRenderer->SetLightConstants(m_sunDirection, m_sunIntensity, m_ambientIntensity);
}

void Game::UpdateLighting()
{
	bool needUpdate = false;
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		needUpdate = true;
		m_sunDirection.x = m_sunDirection.x - 1.f;
		DebugAddMessage(Stringf("Sun direction x: %.1f", m_sunDirection.x), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F3))
	{
		needUpdate = true;
		m_sunDirection.x = m_sunDirection.x + 1.f;
		DebugAddMessage(Stringf("Sun direction x: %.1f", m_sunDirection.x), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F4))
	{
		needUpdate = true;
		m_sunDirection.y = m_sunDirection.y - 1.f;
		DebugAddMessage(Stringf("Sun direction y: %.1f", m_sunDirection.y), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F5))
	{
		needUpdate = true;
		m_sunDirection.y = m_sunDirection.y + 1.f;
		DebugAddMessage(Stringf("Sun direction y: %.1f", m_sunDirection.y), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
	{
		needUpdate = true;
		m_sunIntensity -= 0.05f;
		m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
		DebugAddMessage(Stringf("Sun intensity: %.2f", m_sunIntensity), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		needUpdate = true;
		m_sunIntensity += 0.05f;
		m_sunIntensity = GetClampedZeroToOne(m_sunIntensity);
		DebugAddMessage(Stringf("Sun intensity: %.2f", m_sunIntensity), 2.f);
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		needUpdate = true;
		m_ambientIntensity -= 0.05f;
		m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
		DebugAddMessage(Stringf("Ambient Intensity: %.2f", m_ambientIntensity), 2.f);

	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		needUpdate = true;
		m_ambientIntensity += 0.05f;
		m_ambientIntensity = GetClampedZeroToOne(m_ambientIntensity);
		DebugAddMessage(Stringf("Ambient Intensity: %.2f", m_ambientIntensity), 2.f);
	}

	if (needUpdate)
	{
		ApplyLighting();
	}


}

//-----------------------------------------------------------------------------------------------
void Game::SetNextState(GameState newState)
{
	m_nextState = newState;
}

bool Game::IsCurrentState(GameState state) const
{
	return state == m_currentState;
}

void Game::SwitchToNextState()
{
	if (m_currentState != m_nextState)
	{
		ExitState(m_currentState);
		m_currentState = m_nextState;
		EnterState(m_nextState);
	}
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		PlayMusic(MAIN_MENU_MUSIC, MUSIC_VOLUME);
		PauseSpecialMusic();
		SetPlayBackSpeed(1.f); // Redundant setup
		break;
	case GameState::LOBBY:
		break;
	case GameState::PLAYING:
		PlayMusic(GAME_MUSIC, MUSIC_VOLUME);
		SetPlayBackSpeed(1.f); // Redundant setup
		g_theAudio->StartSound(Sound::SLAYER_ACTIVATED);
		CreatePlayers();
		CreateMaps();
		break;
	case GameState::VICTORY:
		PlayMusic(VICTORY_MUSIC, MUSIC_VOLUME);
		break;
	case GameState::DEFAULT:
		break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		break;
	case GameState::LOBBY:
		break;
	case GameState::PLAYING:
		DestroyMaps();
		DestroyPlayers();
		break;
	case GameState::VICTORY:
		break;
	case GameState::DEFAULT:
		break;
	}
}

void Game::UpdateCurrentState()
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
		UpdateStateAttract();
		break;
	case GameState::LOBBY:
		UpdateStateLobby();
		break;
	case GameState::PLAYING:
		UpdateStatePlaying();
		break;
	case GameState::VICTORY:
		UpdateStateVictory();
		break;
	case GameState::DEFAULT:
		break;
	}
}

void Game::RenderCurrentState() const
{
	switch (m_currentState)
	{
	case GameState::ATTRACT:
		RenderStateAttract();
		break;
	case GameState::LOBBY:
		RenderStateLobby();
		break;
	case GameState::PLAYING:
		RenderStatePlaying();
		break;
	case GameState::VICTORY:
		RenderStateVictory();
		break;
	case GameState::DEFAULT:
		break;
	}
}

void Game::UpdateStateAttract()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK))
	{
		g_theApp->HandleQuitRequested();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		m_isPlayer1UsingKeyBoard = true;
		m_isPlayer1Taken = true;
		m_isPlayer2Taken = false;
		SetNextState(GameState::LOBBY);
	}
	else if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		m_isPlayer1UsingKeyBoard = false;
		m_isPlayer1Taken = true;
		m_isPlayer2Taken = false;
		SetNextState(GameState::LOBBY);
	}

	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::UpdateStateLobby()
{
	XboxController const& controller = g_theInput->GetController(0);
	//-----------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		if (m_isPlayer1UsingKeyBoard)
		{
			m_isPlayer1Taken = false;
		}
		else
		{
			m_isPlayer2Taken = false;
		}
	}
	//-----------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		if (m_isPlayer1UsingKeyBoard)
		{
			if (m_isPlayer1Taken)
			{
				SetNextState(GameState::PLAYING);
			}
			else
			{
				m_isPlayer1Taken = true;
			}
		}
		else
		{
			if (m_isPlayer2Taken)
			{
				SetNextState(GameState::PLAYING);
			}
			else
			{
				m_isPlayer2Taken = true;
			}
		}
	}
	//-----------------------------------------------------------------------------------------------
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		if (!m_isPlayer1UsingKeyBoard)
		{
			m_isPlayer1Taken = false;
		}
		else
		{
			m_isPlayer2Taken = false;
		}
	}
	//-----------------------------------------------------------------------------------------------
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		if (!m_isPlayer1UsingKeyBoard)
		{
			if (m_isPlayer1Taken)
			{
				SetNextState(GameState::PLAYING);
			}
			else
			{
				m_isPlayer1Taken = true;
			}
		}
		else
		{
			if (m_isPlayer2Taken)
			{
				SetNextState(GameState::PLAYING);
			}
			else
			{
				m_isPlayer2Taken = true;
			}
		}
	}

	// no player, back to attract screen
	if (!m_isPlayer1Taken && !m_isPlayer2Taken)
	{
		SetNextState(GameState::ATTRACT);
	}
}

void Game::UpdateStatePlaying()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK))
	{
		g_theAudio->StartSound(BUTTOM_CLICK_SOUND);
		SetNextState(GameState::ATTRACT);
	}
	DebugDrawUpdate();
	UpdateLighting();

	float deltaSeconds = static_cast<float>(m_clock->GetDeltaSeconds());
	for (int playerIndex = 0; playerIndex < (int)g_theGame->m_players.size(); ++playerIndex)
	{
		Player* player = m_players[playerIndex];
		player->Update(deltaSeconds);
	}

	m_currentMap->Update();

	UpdateCameras();

	if (m_currentMap->IsGameFinished())
	{
		SetNextState(GameState::VICTORY);
	}
}

void Game::UpdateStateVictory()
{
	XboxController const& controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) ||
		g_theInput->WasKeyJustPressed(KEYCODE_ESCAPE) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_START) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_BACK) ||
		controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_A))
	{
		SetNextState(GameState::ATTRACT);
	}
}

void Game::RenderStateAttract() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(200, 200, 200));
	Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomCrossing_0.png");
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(verts);




	std::vector<Vertex_PCU> textVerts;
	BitmapFont* font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	std::string text = Stringf(ATTRACT_SCREEN_TEXT);
	AABB2 box(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	box.ChopOffTop(0.8f);
	font->AddVertsForTextInBox2D(textVerts, text, box, 25.f, Rgba8::OPAQUE_WHITE, 0.7f);
	g_theRenderer->BindTexture(&font->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderStateLobby() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	if (m_isPlayer1Taken && m_isPlayer2Taken)
	{
		// 2 players
		std::vector<Vertex_PCU> verts;
		AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(200, 200, 200));
		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomCrossing_3.png");
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(texture);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(verts);
	}
	else if (m_isPlayer1Taken)
	{
		// Player 1
		std::vector<Vertex_PCU> verts;
		AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(200, 200, 200));
		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomCrossing_1.png");
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(texture);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(verts);
	}
	else if (m_isPlayer2Taken)
	{
		// Player 2
		std::vector<Vertex_PCU> verts;
		AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(200, 200, 200));
		Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomCrossing_2.png");
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(texture);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(verts);
	}

	std::vector<Vertex_PCU> textVerts;
	BitmapFont* font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	g_theRenderer->BindTexture(&font->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);

	AABB2 box(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	float aspectRatio = 0.8f;
	Rgba8 color = Rgba8::OPAQUE_WHITE;
	if (m_isPlayer1Taken && m_isPlayer2Taken)
	{
		// 2 players
		font->AddVertsForTextInBox2D(textVerts, "Player 1", box, 60.f, color, aspectRatio, Vec2(0.5f, 0.75f));
		if (m_isPlayer1UsingKeyBoard)
		{
			font->AddVertsForTextInBox2D(textVerts, "Mouse and Keyboard",			box, 30.f, color, aspectRatio, Vec2(0.5f, 0.68f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.60f));
			font->AddVertsForTextInBox2D(textVerts, "Press ESCAPE to leave game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.57f));
		}
		else
		{
			font->AddVertsForTextInBox2D(textVerts, "Controller",					box, 30.f, color, aspectRatio, Vec2(0.5f, 0.68f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.60f));
			font->AddVertsForTextInBox2D(textVerts, "Press BACK to leave game",		box, 15.f, color, aspectRatio, Vec2(0.5f, 0.57f));
		}

		font->AddVertsForTextInBox2D(textVerts, "Player 2",							box, 60.f, color, aspectRatio, Vec2(0.5f, 0.25f));
		if (!m_isPlayer1UsingKeyBoard)
		{
			font->AddVertsForTextInBox2D(textVerts, "Mouse and Keyboard",			box, 30.f, color, aspectRatio, Vec2(0.5f, 0.18f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.10f));
			font->AddVertsForTextInBox2D(textVerts, "Press ESCAPE to leave game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.07f));
		}
		else
		{
			font->AddVertsForTextInBox2D(textVerts, "Controller",					box, 30.f, color, aspectRatio, Vec2(0.5f, 0.18f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.10f));
			font->AddVertsForTextInBox2D(textVerts, "Press BACK to leave game",		box, 15.f, color, aspectRatio, Vec2(0.5f, 0.07f));
		}
	}
	else if (m_isPlayer1Taken)
	{
		// Player 1
		font->AddVertsForTextInBox2D(textVerts, "Player 1",							box, 60.f, color, aspectRatio);
		if (m_isPlayer1UsingKeyBoard)
		{
			font->AddVertsForTextInBox2D(textVerts, "Mouse and Keyboard",			box, 30.f, color, aspectRatio, Vec2(0.5f, 0.43f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.35f));
			font->AddVertsForTextInBox2D(textVerts, "Press ESCAPE to leave game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.32f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to join player",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.29f));
		}
		else
		{
			font->AddVertsForTextInBox2D(textVerts, "Controller",					box, 30.f, color, aspectRatio, Vec2(0.5f, 0.43f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.35f));
			font->AddVertsForTextInBox2D(textVerts, "Press BACK to leave game",		box, 15.f, color, aspectRatio, Vec2(0.5f, 0.32f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to join Player",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.29f));
		}
	}
	else if (m_isPlayer2Taken)
	{
		// Player 2
		font->AddVertsForTextInBox2D(textVerts, "Player 2",							box, 60.f, color, aspectRatio);
		if (!m_isPlayer1UsingKeyBoard)
		{
			font->AddVertsForTextInBox2D(textVerts, "Mouse and Keyboard",			box, 30.f, color, aspectRatio, Vec2(0.5f, 0.43f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.35f));
			font->AddVertsForTextInBox2D(textVerts, "Press ESCAPE to leave game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.32f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to join player",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.29f));
		}
		else
		{
			font->AddVertsForTextInBox2D(textVerts, "Controller",					box, 30.f, color, aspectRatio, Vec2(0.5f, 0.43f));
			font->AddVertsForTextInBox2D(textVerts, "Press START to start game",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.35f));
			font->AddVertsForTextInBox2D(textVerts, "Press BACK to leave game",		box, 15.f, color, aspectRatio, Vec2(0.5f, 0.32f));
			font->AddVertsForTextInBox2D(textVerts, "Press SPACE to join Player",	box, 15.f, color, aspectRatio, Vec2(0.5f, 0.29f));
		}
	}
	g_theRenderer->DrawVertexArray(textVerts);
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderStatePlaying() const
{
	for (int playerIndex = 0; playerIndex < (int)m_players.size(); ++playerIndex)
	{
		m_currentRenderingPlayer = m_players[playerIndex];
		g_theRenderer->BeginCamera(m_currentRenderingPlayer->m_camera);
		m_skybox->Render(m_currentRenderingPlayer->m_camera);
		m_currentMap->Render();
		g_theRenderer->EndCamera(m_currentRenderingPlayer->m_camera);

		DebugRenderWorld(m_currentRenderingPlayer->m_camera);

		g_theRenderer->BeginCamera(m_currentRenderingPlayer->m_screenCamera);
		m_currentRenderingPlayer->RenderScreen();
		g_theRenderer->EndCamera(m_currentRenderingPlayer->m_screenCamera);
	}


	g_theRenderer->BeginCamera(m_screenCamera);
	RenderUI();
	m_currentMap->RenderScreen();
	g_theRenderer->EndCamera(m_screenCamera);
	DebugRenderScreen(m_screenCamera);
}

void Game::RenderStateVictory() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(200, 200, 200));
	Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomDarkAge_0.png");
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(texture);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(verts);




	std::vector<Vertex_PCU> textVerts;
	BitmapFont* font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
	std::string text = Stringf("YOU ARE THE DOOM SLAYER!");
	AABB2 box(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	box.ChopOffTop(0.8f);
	font->AddVertsForTextInBox2D(textVerts, text, box, 25.f, Rgba8::OPAQUE_WHITE, 0.7f);
	g_theRenderer->BindTexture(&font->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_screenCamera);
}

