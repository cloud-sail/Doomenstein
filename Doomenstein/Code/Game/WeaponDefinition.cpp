#include "Game/WeaponDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/IntVec2.hpp"

std::vector<WeaponDefinition*> WeaponDefinition::s_definitions;

STATIC WeaponDefinition const* WeaponDefinition::GetByName(std::string const& weaponDefName)
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		if (s_definitions[i]->m_name == weaponDefName)
		{
			return s_definitions[i];
		}
	}

	ERROR_AND_DIE(Stringf("Weapon is not in WeaponDefs: \"%s\"", weaponDefName.c_str()));
}


STATIC void WeaponDefinition::InitializeDefinitions(const char* path /*= "Data/Definitions/WeaponDefinitions.xml"*/)
{
	ClearDefinitions();

	XmlDocument document;
	XmlResult result = document.LoadFile(path);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open xml file: \"%s\"", path));

	XmlElement* rootElement = document.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("No elements in xml file: \"%s\"", path));

	XmlElement* weaponDefElement = rootElement->FirstChildElement();
	while (weaponDefElement != nullptr)
	{
		std::string elementName = weaponDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "WeaponDefinition", Stringf("Root child element in %s was <%s>, must be <WeaponDefinition>!", path, elementName.c_str()));

		WeaponDefinition* newWeaponDef = new WeaponDefinition();
		newWeaponDef->LoadFromXmlElement(*weaponDefElement);
		s_definitions.push_back(newWeaponDef);

		weaponDefElement = weaponDefElement->NextSiblingElement();
	}
}

STATIC void WeaponDefinition::ClearDefinitions()
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		delete s_definitions[i];
	}
	s_definitions.clear();
}


bool WeaponDefinition::LoadFromXmlElement(XmlElement const& element)
{
	m_name			= ParseXmlAttribute(element, "name", m_name);
	m_refireTime	= ParseXmlAttribute(element, "refireTime", m_refireTime);

	m_rayCount		= ParseXmlAttribute(element, "rayCount", m_rayCount);
	m_rayCone		= ParseXmlAttribute(element, "rayCone", m_rayCone);
	m_rayRange		= ParseXmlAttribute(element, "rayRange", m_rayRange);
	m_rayDamage		= ParseXmlAttribute(element, "rayDamage", m_rayDamage);
	m_rayImpulse	= ParseXmlAttribute(element, "rayImpulse", m_rayImpulse);

	m_projectileCount	= ParseXmlAttribute(element, "projectileCount", m_projectileCount);
	m_projectileCone	= ParseXmlAttribute(element, "projectileCone", m_projectileCone);
	m_projectileSpeed	= ParseXmlAttribute(element, "projectileSpeed", m_projectileSpeed);
	m_projectileActor	= ParseXmlAttribute(element, "projectileActor", m_projectileActor);

	m_meleeCount	= ParseXmlAttribute(element, "meleeCount", m_meleeCount);
	m_meleeArc		= ParseXmlAttribute(element, "meleeArc", m_meleeArc);
	m_meleeRange	= ParseXmlAttribute(element, "meleeRange", m_meleeRange);
	m_meleeDamage	= ParseXmlAttribute(element, "meleeDamage", m_meleeDamage);
	m_meleeImpulse	= ParseXmlAttribute(element, "meleeImpulse", m_meleeImpulse);

	m_switchWeaponSeconds = ParseXmlAttribute(element, "switchWeaponTime", m_switchWeaponSeconds);
	m_specialMeleeCount = ParseXmlAttribute(element, "specialMeleeCount", m_specialMeleeCount);
	m_dashImpulse = ParseXmlAttribute(element, "dashImpulse", m_dashImpulse);
	m_specialRadarCount = ParseXmlAttribute(element, "specialRadarCount", m_specialRadarCount);

	XmlElement const* hudElement = element.FirstChildElement("HUD");
	if (hudElement)
	{
		m_hudShader = g_theRenderer->CreateOrGetShader(ParseXmlAttribute(*hudElement, "shader", "Default").c_str()); // Most of time it is default and using Vertex_PCU, if want to use other, please create before weaponDef Initialzation
		m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(*hudElement, "baseTexture", "").c_str());
		m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(*hudElement, "reticleTexture", "").c_str());
		m_reticleSize = ParseXmlAttribute(*hudElement, "reticleSize", m_reticleSize);
		m_spriteSize = ParseXmlAttribute(*hudElement, "spriteSize", m_spriteSize);
		m_spriteSize = ParseXmlAttribute(*hudElement, "spriteSize", m_spriteSize);

		XmlElement const* animElement = hudElement->FirstChildElement("Animation");
		while (animElement)
		{
			WeaponAnimationInfo* anim = new WeaponAnimationInfo();
			anim->m_name = ParseXmlAttribute(*animElement, "name", "");
			anim->m_shader = g_theRenderer->CreateOrGetShader(ParseXmlAttribute(*animElement, "shader", "Default").c_str());
			Texture* texture = g_theRenderer->CreateOrGetTextureFromFile(ParseXmlAttribute(*animElement, "spriteSheet", "").c_str());
			IntVec2 cellLayout = ParseXmlAttribute(*animElement, "cellCount", IntVec2(1, 1));
			anim->m_spriteSheet = new SpriteSheet(*texture, cellLayout);

			float secondsPerFrame = ParseXmlAttribute(*animElement, "secondsPerFrame", 1.f);
			float framesPerSecond = 1.f / secondsPerFrame;
			int startFrame = ParseXmlAttribute(*animElement, "startFrame", 0);
			int endFrame = ParseXmlAttribute(*animElement, "endFrame", 0);
			if (m_animations.empty())
			{
				anim->m_spriteAnimDef = new SpriteAnimDefinition(*anim->m_spriteSheet, startFrame, endFrame, framesPerSecond, SpriteAnimPlaybackType::LOOP); // IMPORTANT assume first is loop and others are once
			}
			else
			{
				anim->m_spriteAnimDef = new SpriteAnimDefinition(*anim->m_spriteSheet, startFrame, endFrame, framesPerSecond, SpriteAnimPlaybackType::ONCE); // IMPORTANT assume first is loop and others are once
			}

			m_animations.push_back(anim);

			animElement = animElement->NextSiblingElement("Animation");
		}

	}



	XmlElement const* soundsElement = element.FirstChildElement("Sounds");
	if (soundsElement)
	{
		XmlElement const* soundElement = soundsElement->FirstChildElement("Sound");
		while (soundElement)
		{
			std::string soundName = ParseXmlAttribute(*soundElement, "sound", "");
			std::string filePath = ParseXmlAttribute(*soundElement, "name", "");

			if (soundName == "Fire")
			{
				m_fireSFX = g_theAudio->CreateOrGetSound(filePath);
			}

			soundElement = soundElement->NextSiblingElement();
		}
	}

	return true;
}

WeaponAnimationInfo* WeaponDefinition::GetAnimationInfoByName(std::string const& name) const
{
	for (int index = 0; index < (int)m_animations.size(); ++index)
	{
		WeaponAnimationInfo* animInfo = m_animations[index];
		if (animInfo->m_name == name)
		{
			return animInfo;
		}
	}

	return nullptr;
}

WeaponAnimationInfo* WeaponDefinition::GetDefaultAnimationInfo() const
{
	if (m_animations.empty())
	{
		return nullptr;
	}

	return m_animations[0];
}
