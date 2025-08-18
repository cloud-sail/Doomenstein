#include "Game/ActorDefinition.hpp"
#include "Engine/Renderer/Renderer.hpp"



//-----------------------------------------------------------------------------------------------
bool ActorDefinition::CollisionInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_physicsRadius			= ParseXmlAttribute(element, "radius", m_physicsRadius);
	m_physicsHeight			= ParseXmlAttribute(element, "height", m_physicsHeight);
	m_collidesWithWorld		= ParseXmlAttribute(element, "collidesWithWorld", m_collidesWithWorld);
	m_collidesWithActors	= ParseXmlAttribute(element, "collidesWithActors", m_collidesWithActors);
	m_dieOnCollide			= ParseXmlAttribute(element, "dieOnCollide", m_dieOnCollide);
	m_damageOnCollide		= ParseXmlAttribute(element, "damageOnCollide", m_damageOnCollide);
	m_impulseOnCollide		= ParseXmlAttribute(element, "impulseOnCollide", m_impulseOnCollide);
	return true;
}

bool ActorDefinition::PhysicsInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_simulated		= ParseXmlAttribute(element, "simulated", m_simulated);
	m_flying		= ParseXmlAttribute(element, "flying", m_flying);
	m_walkSpeed		= ParseXmlAttribute(element, "walkSpeed", m_walkSpeed);
	m_runSpeed		= ParseXmlAttribute(element, "runSpeed", m_runSpeed);
	m_drag			= ParseXmlAttribute(element, "drag", m_drag);
	m_turnSpeed		= ParseXmlAttribute(element, "turnSpeed", m_turnSpeed);
	return true;
}


bool ActorDefinition::CameraInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_eyeHeight			= ParseXmlAttribute(element, "eyeHeight", m_eyeHeight);
	m_cameraFOVDegrees	= ParseXmlAttribute(element, "cameraFOV", m_cameraFOVDegrees);
	return true;
}

bool ActorDefinition::AIInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_aiEnabled		= ParseXmlAttribute(element, "aiEnabled", m_aiEnabled);
	m_sightRadius	= ParseXmlAttribute(element, "sightRadius", m_sightRadius);
	m_sightAngle = ParseXmlAttribute(element, "sightAngle", m_sightAngle);
	m_staggerSeconds = ParseXmlAttribute(element, "staggerSeconds", m_staggerSeconds);
	m_staggerDamageThreshold = ParseXmlAttribute(element, "staggerDamageThreshold", m_staggerDamageThreshold);
	return true;
}

bool ActorDefinition::InventoryInfo::LoadFromXmlElement(XmlElement const& element)
{
	XmlElement const* weaponElement = element.FirstChildElement("Weapon");
	while (weaponElement)
	{
		std::string weaponName = ParseXmlAttribute(*weaponElement, "name", "");
		if (!weaponName.empty())
		{
			m_weapons.push_back(weaponName);
		}
		weaponElement = weaponElement->NextSiblingElement("Weapon");
	}
	return true;
}


//-----------------------------------------------------------------------------------------------
std::vector<ActorDefinition*> ActorDefinition::s_definitions;

STATIC void ActorDefinition::InitializeDefinitions(std::vector<std::string> const& paths /*= {"Data/Definitions/ActorDefinitions.xml", "Data/Definitions/ProjectileActorDefinitions.xml"}*/)
{
	ClearDefinitions();

	for (std::string const& pathStr : paths)
	{
		char const* path = pathStr.c_str();
		XmlDocument document;
		XmlResult result = document.LoadFile(path);
		GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open xml file: \"%s\"", path));

		XmlElement* rootElement = document.RootElement();
		GUARANTEE_OR_DIE(rootElement, Stringf("No elements in xml file: \"%s\"", path));

		XmlElement* actorDefElement = rootElement->FirstChildElement();
		while (actorDefElement != nullptr)
		{
			std::string elementName = actorDefElement->Name();
			GUARANTEE_OR_DIE(elementName == "ActorDefinition", Stringf("Root child element in %s was <%s>, must be <ActorDefinition>!", path, elementName.c_str()));

			ActorDefinition* newActorDef = new ActorDefinition();
			newActorDef->LoadFromXmlElement(*actorDefElement);
			s_definitions.push_back(newActorDef);

			actorDefElement = actorDefElement->NextSiblingElement();
		}
	}
}

STATIC void ActorDefinition::ClearDefinitions()
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		delete s_definitions[i];
	}
	s_definitions.clear();
}

STATIC ActorDefinition const* ActorDefinition::GetByName(std::string const& actorDefName)
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		if (s_definitions[i]->m_name == actorDefName)
		{
			return s_definitions[i];
		}
	}

	ERROR_AND_DIE(Stringf("Actor is not in ActorDefs: \"%s\"", actorDefName.c_str()));
}


bool ActorDefinition::LoadFromXmlElement(XmlElement const& element)
{
	m_name				= ParseXmlAttribute(element, "name", m_name);
	m_isVisible			= ParseXmlAttribute(element, "visible", m_isVisible);
	m_health			= ParseXmlAttribute(element, "health", m_health);
	m_corpseLifetime	= ParseXmlAttribute(element, "corpseLifetime", m_corpseLifetime);
	m_faction			= StringToFaction(ParseXmlAttribute(element, "faction", "Neutral"));
	m_canBePossessed	= ParseXmlAttribute(element, "canBePossessed", m_canBePossessed);
	m_dieOnSpawn		= ParseXmlAttribute(element, "dieOnSpawn", m_dieOnSpawn);

	XmlElement const* collisionElement = element.FirstChildElement("Collision");
	if (collisionElement)
	{
		m_collision.LoadFromXmlElement(*collisionElement);
	}

	XmlElement const* physicsElement = element.FirstChildElement("Physics");
	if (physicsElement)
	{
		m_physics.LoadFromXmlElement(*physicsElement);
	}

	XmlElement const* cameraElement = element.FirstChildElement("Camera");
	if (cameraElement)
	{
		m_camera.LoadFromXmlElement(*cameraElement);
	}

	XmlElement const* aiElement = element.FirstChildElement("AI");
	if (aiElement)
	{
		m_ai.LoadFromXmlElement(*aiElement);
	}

	XmlElement const* visualsElement = element.FirstChildElement("Visuals");
	if (visualsElement)
	{
		m_visuals.LoadFromXmlElement(*visualsElement);
	}

	XmlElement const* soundsElement = element.FirstChildElement("Sounds");
	if (soundsElement)
	{
		m_sounds.LoadFromXmlElement(*soundsElement);
	}

	XmlElement const* inventoryElement = element.FirstChildElement("Inventory");
	if (inventoryElement)
	{
		m_inventory.LoadFromXmlElement(*inventoryElement);
	}

	return true;
}

ActorDefinition::AnimationGroup* ActorDefinition::GetAnimationGroupByName(std::string const& name) const
{
	for (int index = 0; index < (int)m_visuals.m_animationGroups.size(); ++index)
	{
		AnimationGroup* group = m_visuals.m_animationGroups[index];
		if (group->m_name == name)
		{
			return group;
		}
	}

	return nullptr;
}

ActorDefinition::AnimationGroup* ActorDefinition::GetDefaultAnimationGroup() const
{
	if (m_visuals.m_animationGroups.empty())
	{
		return nullptr;
	}
	return m_visuals.m_animationGroups[0];
}

Faction StringToFaction(std::string const& factionString)
{
	if (factionString == "Marine")
	{
		return Faction::MARINE;
	}
	else if (factionString == "Demon")
	{
		return Faction::DEMON;
	}
	else
	{
		return Faction::NEUTRAL;
	}
}

BillboardType StringToBillboardType(std::string const& billBoardType)
{
	if (billBoardType == "WorldUpOpposing")
	{
		return BillboardType::WORLD_UP_OPPOSING;
	}
	else if (billBoardType == "WorldUpFacing")
	{
		return BillboardType::WORLD_UP_FACING;
	}
	else if (billBoardType == "FullOpposing")
	{
		return BillboardType::FULL_OPPOSING;
	}
	else
	{
		return BillboardType::NONE;
	}
}

SpriteAnimPlaybackType StringToSpriteAnimPlaybackType(std::string const& playbackType)
{
	if (playbackType == "Loop")
	{
		return SpriteAnimPlaybackType::LOOP;
	}
	else if (playbackType == "PingPong")
	{
		return SpriteAnimPlaybackType::PINGPONG;
	}
	else
	{
		return SpriteAnimPlaybackType::ONCE;
	}
}

bool ActorDefinition::SoundsInfo::LoadFromXmlElement(XmlElement const& element)
{
	XmlElement const* soundElement = element.FirstChildElement("Sound");
	while (soundElement)
	{
		std::string soundName = ParseXmlAttribute(*soundElement, "sound", "");
		std::string filePath = ParseXmlAttribute(*soundElement, "name", "");

		if (soundName == "Hurt")
		{
			m_hurtSFX = g_theAudio->CreateOrGetSound(filePath, true);
		}
		else if (soundName == "Death")
		{
			m_deathSFX = g_theAudio->CreateOrGetSound(filePath, true);
		}

		soundElement = soundElement->NextSiblingElement();
	}
	return true;
}

bool ActorDefinition::VisualsInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_size = ParseXmlAttribute(element, "size", m_size);
	m_pivot = ParseXmlAttribute(element, "pivot", m_pivot);
	m_billboardType = StringToBillboardType(ParseXmlAttribute(element, "billboardType", "None"));
	m_renderLit = ParseXmlAttribute(element, "renderLit", m_renderLit);
	m_renderRounded = ParseXmlAttribute(element, "renderRounded", m_renderRounded);

	std::string shaderName = ParseXmlAttribute(element, "shader", "Default");
	m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), VertexType::VERTEX_PCUTBN);

	m_cellCount = ParseXmlAttribute(element, "cellCount", m_cellCount);

	std::string spriteSheetPath = ParseXmlAttribute(element, "spriteSheet", "");
	Texture* spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());
	m_spriteSheet = new SpriteSheet(*spriteSheetTexture, m_cellCount);
	//if (!spriteSheetPath.empty()) // prevent error

	XmlElement const* animationGroupElement = element.FirstChildElement("AnimationGroup");
	while (animationGroupElement)
	{
		AnimationGroup* group = new AnimationGroup();
		group->LoadFromXmlElement(*animationGroupElement, *m_spriteSheet);
		m_animationGroups.push_back(group);
		animationGroupElement = animationGroupElement->NextSiblingElement("AnimationGroup");
	}

	return true;
}

bool ActorDefinition::AnimationGroup::LoadFromXmlElement(XmlElement const& element, SpriteSheet const& spriteSheet)
{
	m_name = ParseXmlAttribute(element, "name", m_name);
	m_scaleBySpeed = ParseXmlAttribute(element, "scaleBySpeed", m_scaleBySpeed);

	float secondsPerFrame = ParseXmlAttribute(element, "secondsPerFrame", 1.f);
	float framesPerSecond = 1.f / secondsPerFrame;
	m_playbackType = StringToSpriteAnimPlaybackType(ParseXmlAttribute(element, "playbackMode", "Once"));
	
	XmlElement const* directionElement = element.FirstChildElement("Direction");
	while (directionElement)
	{
		Vec3 direction = ParseXmlAttribute(*directionElement, "vector", Vec3::FORWARD).GetNormalized();
		m_directions.push_back(direction);

		XmlElement const* animationElement = directionElement->FirstChildElement("Animation");
		GUARANTEE_OR_DIE(animationElement != nullptr, "Missing Animation in Animation Group for the direction");
		int startFrame = ParseXmlAttribute(*animationElement, "startFrame", 0);
		int endFrame = ParseXmlAttribute(*animationElement, "endFrame", 0);
		SpriteAnimDefinition animDef = SpriteAnimDefinition(spriteSheet, startFrame, endFrame, framesPerSecond, m_playbackType);
		m_spriteAnimDefs.push_back(animDef);

		directionElement = directionElement->NextSiblingElement("Direction");
	}

	GUARANTEE_OR_DIE(!m_directions.empty(), "No Direction in Animation Group");
	GUARANTEE_OR_DIE(m_directions.size() == m_spriteAnimDefs.size(), "directions are not match anim defs");

	return true;
}

const SpriteAnimDefinition& ActorDefinition::AnimationGroup::GetAnimationForDirection(Vec3 const& localViewDirection) const
{
	int resultIndex = 0; // no need to check
	float maxDotProduct = DotProduct3D(localViewDirection, m_directions[0]);
	for (int i = 1; i < (int)m_directions.size(); ++i)
	{
		float dotProduct = DotProduct3D(localViewDirection, m_directions[i]);
		if (dotProduct > maxDotProduct)
		{
			resultIndex = i;
			maxDotProduct = dotProduct;
		}
	}

	return m_spriteAnimDefs[resultIndex];

}

bool ActorDefinition::AnimationGroup::IsAnimationFinished(float seconds) const
{
	if (m_playbackType != SpriteAnimPlaybackType::ONCE)
	{
		return false;
	}

	SpriteAnimDefinition const& animDef = m_spriteAnimDefs[0];
	float duration = animDef.GetDuration();
	return seconds > duration;
}
