#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Game/Actor.hpp"
#include "Game/Weapon.hpp"
#include "Game/WeaponDefinition.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"


Player::Player()
{
	m_position = Vec3(2.5f, 8.5f, 0.5f);

	float aspect = g_gameConfigBlackboard.GetValue("windowAspect", 1.777f);
	m_camera.SetPerspectiveView(aspect, 60.f, 0.1f, 100.0f);
	m_camera.SetCameraToRenderTransform(Mat44::DIRECTX_C2R);

	m_hitTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/HitReticle.png");
	m_scannedBoxTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/BoxReticle.png");
	m_speedLinesTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/SpeedLines.png");
	m_lockOnTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CircleReticle.png");
	m_font = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SquirrelFixedFont");
}

void Player::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	// Players are updated in Game Update
	// 
	m_map = g_theGame->GetCurrentMap();
	if (m_map == nullptr)
	{
		return;
	}

	//DebugDrawInfo();
	if (!m_dyingTimer.IsStopped())
	{
		Actor* actor = GetActor();
		if (actor)
		{
			float t = (float)m_dyingTimer.GetElapsedFraction();
			t = GetClampedZeroToOne(t);
			Vec3 startPos = actor->GetEyePosition();
			float z = Interpolate(startPos.z, 0.f, t);
			m_position = Vec3(startPos.x, startPos.y, z);
		}

		UpdateCamera();
		return;
	}

	if (m_thirdPersonTimer > 0.f)
	{
		m_thirdPersonTimer -= deltaSeconds;
	}

	m_hitTrauma -= deltaSeconds * 1.5f;
	m_hitTrauma = GetClampedZeroToOne(m_hitTrauma);

	m_speedLinesTrauma -= deltaSeconds * 2.f;
	m_speedLinesTrauma = GetClampedZeroToOne(m_speedLinesTrauma);

	m_radarScanTrauma -= deltaSeconds * 0.2f;
	m_radarScanTrauma = GetClampedZeroToOne(m_radarScanTrauma);

	if (m_radarScanTrauma > 0.f)
	{
		UpdateRadarScanResult();
	}

	UpdateSinglePlayerCheats();
	UpdateInput();

	if (GetActor() == nullptr)
	{
		m_isFreeFlyMode = true; // if controlled actor is destroyed return to fly mode
	}

	UpdateCamera();
	UpdateAudioListener();
	UpdateAnimation();
	UpdateWeaponShake();

}

void Player::Possess(Actor* newActor)
{
	if (newActor)
	{
		// Reset Player Controller
		float fov = newActor->m_definition->m_camera.m_cameraFOVDegrees;
		float aspect = g_theGame->GetPlayerCameraAspect();
		m_camera.SetPerspectiveView(aspect, fov, CAMERA_ZNEAR, CAMERA_ZFAR);
		m_dyingTimer.Stop();
	}

	Controller::Possess(newActor);
}

Controller::Type Player::GetType() const
{
	return Type::PLAYER;
}

void Player::RenderScreen() const
{
	if (m_isFreeFlyMode)
	{
		return;
	}
	if (IsUsingThirdPersonCamera())
	{
		return;
	}

	Actor* controlledActor = GetActor();

	if (controlledActor == nullptr)
	{
		return;
	}

	// Important only render screen when control marine
	if (controlledActor->m_definition->m_faction != Faction::MARINE)
	{
		return;
	}

	Weapon* currentWeapon = controlledActor->GetEquippedWeapon();

	if (currentWeapon == nullptr)
	{
		return;
	}

	AABB2 cameraBounds = AABB2(m_screenCamera.GetOrthographicBottomLeft(), m_screenCamera.GetOrthographicTopRight());
	
	constexpr float HUD_RATIO = 0.15f;
	constexpr float TEXTBOX_BOTTOM_RATIO = 0.05f;
	constexpr float TEXTBOX_HEIGHT_RATIO = 0.05f;
	constexpr float TEXTBOX_WIDTH_RATIO = 0.125f;
	constexpr float	HEALTH_X_OFFSET = 0.245f;
	AABB2 hudBounds = AABB2(cameraBounds.GetPointAtUV(Vec2(0.f, 0.f)), cameraBounds.GetPointAtUV(Vec2(1.f, HUD_RATIO)));
	AABB2 speedLineBounds = AABB2(cameraBounds.GetPointAtUV(Vec2(0.f, HUD_RATIO)), cameraBounds.GetPointAtUV(Vec2(1.f, 1.f)));
	AABB2 killsBounds	= AABB2(cameraBounds.GetPointAtUV(Vec2(0.f, TEXTBOX_BOTTOM_RATIO)), cameraBounds.GetPointAtUV(Vec2(TEXTBOX_WIDTH_RATIO, TEXTBOX_BOTTOM_RATIO + TEXTBOX_HEIGHT_RATIO)));
	AABB2 deathsBounds	= AABB2(cameraBounds.GetPointAtUV(Vec2(1.f - TEXTBOX_WIDTH_RATIO, TEXTBOX_BOTTOM_RATIO)), cameraBounds.GetPointAtUV(Vec2(1.f, TEXTBOX_BOTTOM_RATIO + TEXTBOX_HEIGHT_RATIO)));
	AABB2 healthBounds = AABB2(cameraBounds.GetPointAtUV(Vec2(HEALTH_X_OFFSET, TEXTBOX_BOTTOM_RATIO)), cameraBounds.GetPointAtUV(Vec2(TEXTBOX_WIDTH_RATIO + HEALTH_X_OFFSET, TEXTBOX_BOTTOM_RATIO + TEXTBOX_HEIGHT_RATIO)));

	if (!controlledActor->m_isDead && currentWeapon->m_currentAnimation != nullptr)
	{
		// Render weapon sprite
		Vec2 weaponSpriteSize = currentWeapon->m_definition->m_spriteSize;
		weaponSpriteSize *= cameraBounds.GetDimensions().y / SCREEN_SIZE_Y;
		AABB2 weaponBounds = AABB2(Vec2::ZERO, weaponSpriteSize);
		weaponBounds.Translate(weaponSpriteSize * -currentWeapon->m_definition->m_spritePivot);
		weaponBounds.Translate(hudBounds.GetPointAtUV(Vec2(0.5f, 1.f)));
		
		float fraction = currentWeapon->GetElapsedFractionOfSwitchWeapon();
		weaponBounds.Translate(Vec2(0.f, (fraction - 1.f) * weaponSpriteSize.y));
		weaponBounds.Translate(m_weaponShakeOffset * weaponSpriteSize.y);

		std::vector<Vertex_PCU> weaponVerts;
		AddVertsForAABB2D(weaponVerts, weaponBounds, Rgba8::OPAQUE_WHITE, currentWeapon->GetCurrentSpriteUV());

		g_theRenderer->BindTexture(&currentWeapon->m_currentAnimation->m_spriteSheet->GetTexture());
		g_theRenderer->BindShader(currentWeapon->m_currentAnimation->m_shader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(weaponVerts);


		// Render reticle
		Vec2 reticleSize = currentWeapon->m_definition->m_reticleSize;
		reticleSize *= cameraBounds.GetDimensions().y / SCREEN_SIZE_Y;
		AABB2 reticleBounds = AABB2(Vec2::ZERO, reticleSize);
		reticleBounds.SetCenter(cameraBounds.GetCenter());
		
		std::vector<Vertex_PCU> reticleVerts;
		AddVertsForAABB2D(reticleVerts, reticleBounds, Rgba8::OPAQUE_WHITE);

		g_theRenderer->BindTexture(currentWeapon->m_definition->m_reticleTexture);
		g_theRenderer->BindShader(currentWeapon->m_definition->m_hudShader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(reticleVerts);
		
		// Render hit Reticle
		unsigned char hitAlpha = DenormalizeByte(m_hitTrauma);
		AABB2 hitReticleBounds = reticleBounds;
		hitReticleBounds.SetDimensions(reticleBounds.GetDimensions() * 2.f);
		std::vector<Vertex_PCU> hitReticleVerts;
		AddVertsForAABB2D(hitReticleVerts, hitReticleBounds, Rgba8(255, 255, 255, hitAlpha));

		g_theRenderer->BindTexture(m_hitTexture);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(hitReticleVerts);

	}

	// Render Hud
	std::vector<Vertex_PCU> hudVerts;
	AddVertsForAABB2D(hudVerts, hudBounds, Rgba8::OPAQUE_WHITE);

	g_theRenderer->BindTexture(currentWeapon->m_definition->m_baseTexture);
	g_theRenderer->BindShader(currentWeapon->m_definition->m_hudShader);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(hudVerts);


	std::vector<Vertex_PCU> speedLinesVerts;
	unsigned char speedLinesAlpha = DenormalizeByte(m_speedLinesTrauma);
	AddVertsForAABB2D(speedLinesVerts, speedLineBounds, Rgba8(255, 255, 255, speedLinesAlpha));

	g_theRenderer->BindTexture(m_speedLinesTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->DrawVertexArray(speedLinesVerts);

	// Render Texts (Health, Deaths, Kills)

	std::vector<Vertex_PCU> textVerts;
	g_theRenderer->BindTexture(&m_font->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);

	m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_numKills), killsBounds, 60.f);
	if (m_numDeaths == 0)
	{
		m_font->AddVertsForTextInBox2D(textVerts, "NEVER", deathsBounds, 60.f);
	}
	else
	{
		m_font->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_numDeaths), deathsBounds, 60.f);
	}
	m_font->AddVertsForTextInBox2D(textVerts, Stringf("%.0f", controlledActor->m_health), healthBounds, 60.f);

	g_theRenderer->DrawVertexArray(textVerts);


	// show data
	if (m_radarScanTrauma > 0.f)
	{
		std::vector<Vertex_PCU> enemyScreenPointVerts;
		std::vector<Vertex_PCU> enemyDescriptionVerts;

		for (int i = 0; i < (int)m_scannedEnemyViewportPoints.size(); ++i)
		{
			Vec2 screenPos = cameraBounds.GetPointAtUV(m_scannedEnemyViewportPoints[i]);

			AABB2 lockBounds = AABB2(Vec2::ZERO, Vec2(50.f, 50.f)); // TODO multiplayer smaller?
			lockBounds.SetCenter(screenPos);
			AABB2 descBounds = AABB2(Vec2::ZERO, Vec2(100.f, 50.f));
			descBounds.SetCenter(screenPos + Vec2(0.f, 50.f));
			AddVertsForAABB2D(enemyScreenPointVerts, lockBounds, Rgba8::OPAQUE_WHITE);
			m_font->AddVertsForTextInBox2D(enemyDescriptionVerts, m_scannedEnemyDescriptions[i], descBounds, 25.f, Rgba8::OPAQUE_WHITE, 0.7f, Vec2(0.5f, 0.f));
		}

		g_theRenderer->BindTexture(m_scannedBoxTexture); // Placeholder
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(enemyScreenPointVerts);

		g_theRenderer->BindTexture(&m_font->GetTexture());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(enemyDescriptionVerts);
	}

	if (m_isLockOn)
	{
		std::vector<Vertex_PCU> lockOnVerts;
		Vec2 reticleSize = Vec2(50.f, 50.f);
		reticleSize *= cameraBounds.GetDimensions().y / SCREEN_SIZE_Y;

		AABB2 reticleBounds = AABB2(Vec2::ZERO, reticleSize);
		reticleBounds.SetCenter(cameraBounds.GetCenter());

		AddVertsForAABB2D(lockOnVerts, reticleBounds, Rgba8(0, 255, 30));


		g_theRenderer->BindTexture(m_lockOnTexture); // Placeholder
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(lockOnVerts);
	}
	else
	{
		if (m_map)
		{
			Actor* closestActor = m_map->GetActorByHandle(m_closestToCenterEnemy);
			if (closestActor)
			{
				Vec3 worldPosDemon = closestActor->GetEyePosition();
				Vec2 viewportPos;
				bool isInView = m_camera.ProjectWorldToViewportPoint(worldPosDemon, viewportPos);
				if (isInView)
				{
					Vec2 screenPos = cameraBounds.GetPointAtUV(viewportPos);
					Vec2 reticleSize = Vec2(50.f, 50.f);
					reticleSize *= cameraBounds.GetDimensions().y / SCREEN_SIZE_Y;
					AABB2 reticleBounds = AABB2(Vec2::ZERO, reticleSize);
					reticleBounds.SetCenter(screenPos);

					std::vector<Vertex_PCU> lockOnVerts;
					AddVertsForAABB2D(lockOnVerts, reticleBounds, Rgba8(255, 183, 30));

					g_theRenderer->BindTexture(m_lockOnTexture); // Placeholder
					g_theRenderer->BindShader(nullptr);
					g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
					g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
					g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
					g_theRenderer->SetDepthMode(DepthMode::DISABLED);
					g_theRenderer->DrawVertexArray(lockOnVerts);
				}
			}
		}
	}



	if (!m_dyingTimer.IsStopped())
	{
		std::vector<Vertex_PCU> deathOverlayVerts;
		AABB2 overlayBox = cameraBounds;
		AddVertsForAABB2D(deathOverlayVerts, overlayBox, Rgba8(0, 0, 0, 120));
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->DrawVertexArray(deathOverlayVerts);
	}
}

void Player::OnControlledActorDie()
{
	Actor* actor = GetActor();
	if (actor)
	{
		m_dyingTimer = Timer(actor->m_definition->m_corpseLifetime * 0.5f);
		m_dyingTimer.Start();
	}
}

void Player::SetThirdPersonTimer(float seconds)
{
	m_thirdPersonTimer = seconds;
}

bool Player::IsUsingThirdPersonCamera() const
{
	return m_thirdPersonTimer > 0.f;
}

void Player::AddHitTrauma(float stress)
{
	m_hitTrauma += stress;
	m_hitTrauma = GetClampedZeroToOne(m_hitTrauma);
}

void Player::AddSpeedLinesTrauma(float stress)
{
	m_speedLinesTrauma += stress;
	m_speedLinesTrauma = GetClampedZeroToOne(m_speedLinesTrauma);
}

void Player::AddRadarScanTrauma(float stress)
{
	m_radarScanTrauma += stress;
	m_radarScanTrauma = GetClampedZeroToOne(m_radarScanTrauma);
}

void Player::UpdateInput()
{
	if (m_isFreeFlyMode)
	{
		UnlockTarget();
		UpdateFreeFlyInput(); // both input can be used, since it is single player mode
	}
	else if (IsUsingThirdPersonCamera())
	{
		UnlockTarget();
		UpdateThirdPersonInput();
	}
	else
	{
		UpdateFirstPersonInput();
	}

}

void Player::UpdateFreeFlyInput()
{
	// Can move when the game is paused
	float unscaledDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());
	XboxController const& controller = g_theInput->GetController(0);

	// Sprint
	float const speedMultiplier = (g_theInput->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_A) || controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER)) ? CAMERA_SPEED_FACTOR : 1.f;

	Vec3 moveIntention = Vec3(controller.GetLeftStick().GetPosition().GetRotatedMinus90Degrees());

	if (g_theInput->IsKeyDown(KEYCODE_W))
	{
		moveIntention += Vec3(1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		moveIntention += Vec3(-1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_A))
	{
		moveIntention += Vec3(0.f, 1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		moveIntention += Vec3(0.f, -1.f, 0.f);
	}

	moveIntention.ClampLength(1.f);

	Vec3 forwardIBasis, leftJBasis, upKBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(forwardIBasis, leftJBasis, upKBasis);


	m_position += (forwardIBasis * moveIntention.x + leftJBasis * moveIntention.y + upKBasis * moveIntention.z) *
		CAMERA_MOVE_SPEED * unscaledDeltaSeconds * speedMultiplier;
	

	Vec3 elevateIntention;
	if (g_theInput->IsKeyDown(KEYCODE_Z))
	{
		elevateIntention += Vec3(0.f, 0.f, -1.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_C))
	{
		elevateIntention += Vec3(0.f, 0.f, 1.f);
	}
	if (controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER))
	{
		elevateIntention += Vec3(0.f, 0.f, -1.f);
	}
	if (controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_RIGHT_SHOULDER))
	{
		elevateIntention += Vec3(0.f, 0.f, 1.f);
	}
	elevateIntention.ClampLength(1.f);

	m_position += elevateIntention * CAMERA_MOVE_SPEED * unscaledDeltaSeconds * speedMultiplier;

	//XboxController const& controller = g_theInput->GetController(0);

	// Yaw & Pitch
	Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
	float deltaYaw = -cursorPositionDelta.x * MOUSE_AIM_RATE;
	float deltaPitch = cursorPositionDelta.y * MOUSE_AIM_RATE;

	m_orientation.m_yawDegrees += deltaYaw;
	m_orientation.m_pitchDegrees += deltaPitch;

	Vec2 rightStick = controller.GetRightStick().GetPosition();
	deltaYaw = -unscaledDeltaSeconds * CAMERA_YAW_TURN_RATE * rightStick.x;
	deltaPitch = -unscaledDeltaSeconds * CAMERA_PITCH_TURN_RATE * rightStick.y;

	m_orientation.m_yawDegrees += deltaYaw;
	m_orientation.m_pitchDegrees += deltaPitch;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);


	//-----------------------------------------------------------------------------------------------
	// Alt Move
	Actor* controlledActor = GetActor();
	if (controlledActor)
	{
		Vec3 altMoveIntention;

		if (g_theInput->IsKeyDown(KEYCODE_I))
		{
			altMoveIntention += Vec3(1.f, 0.f, 0.f);
		}
		if (g_theInput->IsKeyDown(KEYCODE_K))
		{
			altMoveIntention += Vec3(-1.f, 0.f, 0.f);
		}
		if (g_theInput->IsKeyDown(KEYCODE_J))
		{
			altMoveIntention += Vec3(0.f, 1.f, 0.f);
		}
		if (g_theInput->IsKeyDown(KEYCODE_L))
		{
			altMoveIntention += Vec3(0.f, -1.f, 0.f);
		}
		altMoveIntention.ClampLength(1.f);
		float altDegreesOffset = altMoveIntention.GetAngleAboutZDegrees();
		Vec3 altMoveDirection = Vec3(Vec2::MakeFromPolarDegrees(m_orientation.m_yawDegrees + altDegreesOffset));
		float altMoveSpeed = controlledActor->m_definition->m_physics.m_runSpeed * altMoveIntention.GetLength();
		controlledActor->MoveInDirection(altMoveDirection, altMoveSpeed);
	}	
}

void Player::UpdateFirstPersonInput()
{
	Actor* controlledActor = GetActor();

	if (controlledActor == nullptr)
	{
		return;
	}

	// Before operate on actor, reset the transform
	m_position = controlledActor->GetEyePosition();
	m_orientation = controlledActor->m_orientation;

	// Update Closest To Center
	Actor* nearestEnemy = GetNearestEnemyToCenter();
	if (nearestEnemy)
	{
		m_closestToCenterEnemy = nearestEnemy->m_handle;
	}
	else
	{
		m_closestToCenterEnemy = ActorHandle::INVALID;
	}


	if (m_controllerIndex == KEYBOARD_AND_MOUSE)
	{
		UpdateFirstPersonInputByKeyboard(controlledActor);
	}
	else
	{
		UpdateFirstPersonInputByController(controlledActor);
	}
}

void Player::UpdateFirstPersonInputByKeyboard(Actor* controlledActor)
{
	//-----------------------------------------------------------------------------------------------
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		AttemptLockOn();
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_RIGHT_MOUSE))
	{
		UnlockTarget();
	}

	if (m_isLockOn && m_map->GetActorByHandle(m_targetActorHandle) == nullptr)
	{
		UnlockTarget();
	}

	//-----------------------------------------------------------------------------------------------
	// Move
	Vec3 moveIntention;

	if (g_theInput->IsKeyDown(KEYCODE_W))
	{
		moveIntention += Vec3(1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		moveIntention += Vec3(-1.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_A))
	{
		moveIntention += Vec3(0.f, 1.f, 0.f);
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		moveIntention += Vec3(0.f, -1.f, 0.f);
	}

	moveIntention.ClampLength(1.f);

	float degreesOffset = moveIntention.GetAngleAboutZDegrees();
	Vec3 moveDirection = Vec3(Vec2::MakeFromPolarDegrees(m_orientation.m_yawDegrees + degreesOffset));
	float moveSpeed = (g_theInput->IsKeyDown(KEYCODE_SHIFT)) ?
		controlledActor->m_definition->m_physics.m_walkSpeed : controlledActor->m_definition->m_physics.m_runSpeed;
	moveSpeed *= moveIntention.GetLength();

	controlledActor->MoveInDirection(moveDirection, moveSpeed);

	//-----------------------------------------------------------------------------------------------
	// Aim
	//float unscaledDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

	if (m_isLockOn)
	{
		Actor* lockedEnemy = m_map->GetActorByHandle(m_targetActorHandle);

		if (lockedEnemy)
		{
			Mat44 rotMat = Mat44::MakeFromX(lockedEnemy->GetEyePosition() - m_position);
			m_orientation = rotMat.GetEulerAngles();
		}
	}
	else
	{
		Vec2 cursorPositionDelta = g_theInput->GetCursorClientDelta();
		float deltaYaw = -cursorPositionDelta.x * MOUSE_AIM_RATE;
		float deltaPitch = cursorPositionDelta.y * MOUSE_AIM_RATE;

		m_orientation.m_yawDegrees += deltaYaw;
		m_orientation.m_pitchDegrees += deltaPitch;
	}
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
	// Update Actor orientation
	controlledActor->m_orientation = m_orientation;



	//-----------------------------------------------------------------------------------------------
	// Fire
	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
	{
		controlledActor->Attack();
	}

	//-----------------------------------------------------------------------------------------------
	// Switch Weapon
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT))
	{
		controlledActor->EquipPrevWeapon();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT))
	{
		controlledActor->EquipNextWeapon();
	}
	if (g_theInput->WasKeyJustPressed('1'))
	{
		controlledActor->EquipWeapon(0);
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		controlledActor->EquipWeapon(1);
	}
	if (g_theInput->WasKeyJustPressed('3'))
	{
		controlledActor->EquipWeapon(2);
	}
}

void Player::UpdateFirstPersonInputByController(Actor* controlledActor)
{
	XboxController const& controller = g_theInput->GetController(m_controllerIndex);
	//-----------------------------------------------------------------------------------------------
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER))
	{
		AttemptLockOn();
	}

	if (controller.WasButtonJustReleased(XboxButtonId::XBOX_BUTTON_LEFT_SHOULDER))
	{
		UnlockTarget();
	}

	if (m_isLockOn && m_map->GetActorByHandle(m_targetActorHandle) == nullptr)
	{
		UnlockTarget();
	}
	//-----------------------------------------------------------------------------------------------
	// Move
	Vec3 moveIntention = Vec3(controller.GetLeftStick().GetPosition().GetRotatedMinus90Degrees());

	moveIntention.ClampLength(1.f);

	float degreesOffset = moveIntention.GetAngleAboutZDegrees();
	Vec3 moveDirection = Vec3(Vec2::MakeFromPolarDegrees(m_orientation.m_yawDegrees + degreesOffset));
	float moveSpeed = (controller.IsButtonDown(XboxButtonId::XBOX_BUTTON_A) || controller.GetLeftTrigger() > 0.f) ?
		controlledActor->m_definition->m_physics.m_walkSpeed : controlledActor->m_definition->m_physics.m_runSpeed;
	moveSpeed *= moveIntention.GetLength();

	controlledActor->MoveInDirection(moveDirection, moveSpeed);

	//-----------------------------------------------------------------------------------------------
	// Aim
	if (m_isLockOn)
	{
		Actor* lockedEnemy = m_map->GetActorByHandle(m_targetActorHandle);

		if (lockedEnemy)
		{
			Mat44 rotMat = Mat44::MakeFromX(lockedEnemy->GetEyePosition() - m_position);
			m_orientation = rotMat.GetEulerAngles();
		}
	}
	else
	{
		float unscaledDeltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

		Vec2 rightStick = controller.GetRightStick().GetPosition();
		float deltaYaw = -unscaledDeltaSeconds * CAMERA_YAW_TURN_RATE * rightStick.x;
		float deltaPitch = -unscaledDeltaSeconds * CAMERA_PITCH_TURN_RATE * rightStick.y;

		m_orientation.m_yawDegrees += deltaYaw;
		m_orientation.m_pitchDegrees += deltaPitch;
	}
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);
	// Update Actor orientation
	controlledActor->m_orientation = m_orientation;

	//-----------------------------------------------------------------------------------------------
	// Fire
	if (controller.GetRightTrigger() > 0.f)
	{
		controlledActor->Attack();
	}

	//-----------------------------------------------------------------------------------------------
	// Switch Weapon
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_UP) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_LEFT))
	{
		controlledActor->EquipPrevWeapon();
	}
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_DOWN) || controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_RIGHT))
	{
		controlledActor->EquipNextWeapon();
	}
	if ( controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_X))
	{
		controlledActor->EquipWeapon(0);
	}
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_Y))
	{
		controlledActor->EquipWeapon(1);
	}
	if (controller.WasButtonJustPressed(XboxButtonId::XBOX_BUTTON_B))
	{
		controlledActor->EquipWeapon(2);
	}
}

void Player::UpdateSinglePlayerCheats()
{
	if (g_theGame->IsMultiplayerMode())
	{
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F))
	{
		if (m_isFreeFlyMode)
		{
			Actor* controlledActor = GetActor();
			if (controlledActor)
			{
				m_isFreeFlyMode = false;
			}
			else
			{
				DebugAddMessage("Controlled Actor has been destroyed, Press N to select the next one!", 5.f, Rgba8::RED, Rgba8::RED);
			}
		}
		else
		{
			m_isFreeFlyMode = true;
		}
	}

	// In Gold Disable Possess next
	//if (g_theInput->WasKeyJustPressed(KEYCODE_N))
	//{
	//	m_map->DebugPossessNext(this);
	//}
}

void Player::UpdateThirdPersonInput()
{
	Actor* controlledActor = GetActor();

	if (controlledActor == nullptr)
	{
		return;
	}

	controlledActor->GetThirdPersonCameraPositionAndOrientation(m_position, m_orientation);
}

void Player::UpdateRadarScanResult()
{
	m_scannedEnemyViewportPoints.clear();
	m_scannedEnemyDescriptions.clear();
	if (m_map)
	{
		//int numActor = (int)m_map->m_allActors.size();
		for (Actor const* actor : m_map->m_allActors)
		{
			if (actor == nullptr || actor->m_isDead)
			{
				continue;
			}
			if (actor->m_definition->m_faction == Faction::DEMON)
			{
				Vec3 worldPos = actor->GetEyePosition();
				Vec2 viewportPos;
				bool isInView = m_camera.ProjectWorldToViewportPoint(worldPos, viewportPos);
				if (isInView && AABB2::ZERO_TO_ONE.IsPointInside(viewportPos))
				{
					float dist = GetDistance3D(worldPos, m_camera.GetPosition());
					//float dist = GetDistance3D(worldPos, m_position);
					m_scannedEnemyViewportPoints.push_back(viewportPos);
					m_scannedEnemyDescriptions.push_back(Stringf("Distance:\n%.1f\nHealth:\n%.0f", dist, actor->m_health));
				}
			}
		}
	}
}

void Player::AttemptLockOn()
{
	if (m_map->GetActorByHandle(m_closestToCenterEnemy) == nullptr)
	{
		return;
	}
	m_targetActorHandle = m_closestToCenterEnemy;
	m_isLockOn = true;
}

void Player::UnlockTarget()
{
	if (!m_isLockOn)
	{
		return;
	}

	m_isLockOn = false;
	m_targetActorHandle = ActorHandle::INVALID;
}

Actor* Player::GetNearestEnemyToCenter() const
{
	if (m_map == nullptr)
	{
		return nullptr;
	}

	std::vector<Actor*> aliveDemons;

	// Gather all demons
	for (Actor* actor : m_map->m_allActors)
	{
		if (actor == nullptr || actor->m_isDead || actor->m_definition->m_faction != Faction::DEMON)
		{
			continue;
		}

		aliveDemons.push_back(actor);
	}


	// Closest demon to the center of the screen
	Actor* closestDemon = nullptr;
	float minDistanceX = 99999.f;

	for (Actor* demon : aliveDemons)
	{
		Vec3 worldPosDemon = demon->GetEyePosition();
		Vec2 viewportPos;
		bool isInView = m_camera.ProjectWorldToViewportPoint(worldPosDemon, viewportPos);
		if (!isInView)
		{
			continue;
		}
		AABB2 lockOnBox = AABB2(Vec2(0.25f, 0.25f), Vec2(0.75f, 0.75f));
		if (!lockOnBox.IsPointInside(viewportPos))
		{
			continue;
		}

		// Fast Raycast to check visibility
		Vec2 rayStart = Vec2(m_camera.GetPosition().x, m_camera.GetPosition().y);
		Vec2 rayEnd = Vec2(worldPosDemon.x, worldPosDemon.y);
		Vec2 disp = rayEnd - rayStart;
		RaycastResult2D result = m_map->FastVoxelRaycast(rayStart, disp.GetNormalized(), disp.GetLength());
		if (result.m_didImpact)
		{
			continue;
		}

		// Closest to center , only takes x because y is not so important
		float distanceX = fabsf(viewportPos.x - 0.5f);
		if (distanceX < minDistanceX)
		{
			closestDemon = demon;
			minDistanceX = distanceX;
		}
	}

	return closestDemon;
}

void Player::UpdateCamera()
{
	m_camera.SetPositionAndOrientation(m_position, m_orientation);
}

void Player::UpdateAudioListener()
{
	Vec3 iBasis, jBasis, kBasis;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(iBasis, jBasis, kBasis);

	g_theAudio->UpdateListener(m_playerIndex, m_position, iBasis, kBasis);
}

void Player::UpdateAnimation()
{
	Actor* controlledActor = GetActor();

	if (controlledActor == nullptr)
	{
		return;
	}

	// Reset to default when other animation is finished
	Weapon* currentWeapon = controlledActor->GetEquippedWeapon();
	if (currentWeapon)
	{
		if (currentWeapon->IsCurrentAnimationFinished())
		{
			currentWeapon->PlayDefaultAnimation();
		}
	}
}

void Player::UpdateWeaponShake()
{
	Actor* controlledActor = GetActor();

	if (controlledActor == nullptr)
	{
		return;
	}

	float currentSpeed = controlledActor->GetVelocity().GetLength();
	float maxSpeed = controlledActor->m_definition->m_physics.m_runSpeed;
	float t = RangeMapClamped(currentSpeed, 0.f, maxSpeed, 0.f, 1.f);

	float radius = t * MAX_WEAPON_SHAKE_RADIUS_RATIO;
	constexpr float STRIDE_PERIOD = 0.8f;
	float degrees = t * MAX_WEAPON_SHAKE_DEGREES * LinearSine((float)g_theGame->m_clock->GetTotalSeconds(), STRIDE_PERIOD);

	Vec2 targetOffset = Vec2::MakeFromPolarDegrees(degrees - 90.f, radius);
	targetOffset.x *= 1.5f;
	constexpr float INTERP_SPEED = 3.f;
	m_weaponShakeOffset = m_weaponShakeOffset + (targetOffset - m_weaponShakeOffset) * GetClampedZeroToOne((float)g_theGame->m_clock->GetDeltaSeconds() * INTERP_SPEED);
}

void Player::DebugDrawInfo()
{
	AABB2 textBox = AABB2(200.f, 400.f, 600.f, 500.f);
	Vec2 alignment = Vec2(0.5f, 0.5);
	float cellHeight = 50.f;
	Actor* actor = GetActor();

	if (actor)
	{
		DebugAddScreenText(Stringf("Player: %s[%d] Health: %.1f", actor->m_definition->m_name.c_str(), actor->m_handle.GetIndex(), actor->m_health), textBox, cellHeight, alignment, 0.f, 0.7f);
	}
	else
	{
		DebugAddScreenText("No actor controlled.", textBox, cellHeight, alignment, 0.f, 0.7f);
	}
}

//void Player::DoRaycast(float distance)
//{
//	Vec3 fwd = m_orientation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
//
//	RaycastResultWithActor raycastResult = g_theGame->GetCurrentMap()->RaycastAll(m_position, fwd, distance);
//	DebugDrawRaycastResult3D(raycastResult);
//
//}

